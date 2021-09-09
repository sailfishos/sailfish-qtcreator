/****************************************************************************
**
** Copyright (C) 2019 Jolla Ltd.
** Copyright (C) 2019-2020 Open Mobile Platform LLC.
** Contact: http://jolla.com/
**
** This file is part of Qt Creator.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Digia.
**
****************************************************************************/

#include "sdkmanager.h"

#include "configuration.h"
#include "dbus.h"
#include "dispatch.h"
#include "remoteprocess.h"
#include "sfdkconstants.h"
#include "sfdkglobal.h"
#include "textutils.h"

#include <sfdk/buildengine.h>
#include <sfdk/device.h>
#include <sfdk/emulator.h>
#include <sfdk/sdk.h>
#include <sfdk/sfdkconstants.h>
#include <sfdk/virtualmachine.h>

#include <mer/merconstants.h>
#include <mer/mersettings.h>
#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QBuffer>
#include <QDir>
#include <QPointer>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QTimer>

#include <bitset>

namespace {
const int CONNECT_TIMEOUT_MS = 60000;
const int LOCK_DOWN_TIMEOUT_MS = 60000;

#ifdef Q_OS_MACOS
const char SDK_MAINTENANCE_TOOL[] = "SDKMaintenanceTool.app/Contents/MacOS/SDKMaintenanceTool";
#else
const char SDK_MAINTENANCE_TOOL[] = "SDKMaintenanceTool" QTC_HOST_EXE_SUFFIX;
#endif

const int SDK_MAINTENANCE_TOOL_EXIT_CANCEL = 3;

const char CUSTOM_PACKAGES_PREFIX[] = "x.";

const char MINIMAL_UPDATES_XML[] = R"(
<Updates>
 <ApplicationName>{AnyApplication}</ApplicationName>
 <ApplicationVersion>1.0.0</ApplicationVersion>
</Updates>
)";

const char TARGET_NAME_FILE[] = ".sfdk/target";

} // namespace anonymous

using namespace Mer;
using namespace Mer::Internal;
using namespace QSsh;
using namespace Sfdk;
using namespace Utils;

namespace Sfdk {

class VmConnectionUi : public VirtualMachine::ConnectionUi
{
    Q_OBJECT

public:
    using VirtualMachine::ConnectionUi::ConnectionUi;

    void warn(Warning which) override
    {
        switch (which) {
        case UnableToCloseVm:
            qerr() << tr("Timeout waiting for the \"%1\" virtual machine to close.")
                .arg(virtualMachine()->name()) << endl;
            break;
        case VmNotRegistered:
            qerr() << tr("No virtual machine with the name \"%1\" found. Check your installation.")
                .arg(virtualMachine()->name()) << endl;
            break;
        case SshPortOccupied:
            qerr() << tr("Another application seems to be listening on the TCP port %1 configured as "
                    "SSH port for the \"%2\" virtual machine - choose another SSH port in options.")
                .arg(virtualMachine()->sshParameters().port())
                .arg(virtualMachine()->name()) << endl;
            break;
        }
    }

    void dismissWarning(Warning which) override
    {
        Q_UNUSED(which);
    }

    bool shouldAsk(Question which) const override
    {
        Q_UNUSED(which);
        return false;
    }

    void ask(Question which, std::function<void()> onStatusChanged) override
    {
        switch (which) {
        case StartVm:
            QTC_CHECK(false);
            break;
        case ResetVm:
            QTC_CHECK(false);
            break;
        case CloseVm:
            QTC_CHECK(false);
            break;
        case CancelConnecting:
            QTC_CHECK(!m_cancelConnectingTimer);
            m_cancelConnectingTimer = startTimer(CONNECT_TIMEOUT_MS, onStatusChanged,
                tr("Timeout connecting to the \"%1\" virtual machine.")
                    .arg(virtualMachine()->name()));
            break;
        case CancelLockingDown:
            QTC_CHECK(!m_cancelLockingDownTimer);
            m_cancelLockingDownTimer = startTimer(LOCK_DOWN_TIMEOUT_MS, onStatusChanged,
                tr("Timeout waiting for the \"%1\" virtual machine to close.")
                    .arg(virtualMachine()->name()));
            break;
        }
}

    void dismissQuestion(Question which) override
    {
        switch (which) {
        case StartVm:
        case ResetVm:
        case CloseVm:
            break;
        case CancelConnecting:
            delete m_cancelConnectingTimer;
            break;
        case CancelLockingDown:
            delete m_cancelLockingDownTimer;
            break;
        }
    }

    QuestionStatus status(Question which) const override
    {
        switch (which) {
        case StartVm:
            QTC_CHECK(false);
            return NotAsked;
        case ResetVm:
            QTC_CHECK(false);
            return NotAsked;
        case CloseVm:
            QTC_CHECK(false);
            return NotAsked;
        case CancelConnecting:
            return status(m_cancelConnectingTimer);
        case CancelLockingDown:
            return status(m_cancelLockingDownTimer);
        }

        QTC_CHECK(false);
        return NotAsked;
    }

private:
    QTimer *startTimer(int timeout, std::function<void()> onStatusChanged, const QString &timeoutMessage)
    {
        QTimer *timer = new QTimer(this);
        connect(timer, &QTimer::timeout, [=] {
            qerr() << timeoutMessage << endl;
            onStatusChanged();
        });
        timer->setSingleShot(true);
        timer->start(timeout);
        return timer;
    }

    static QuestionStatus status(QTimer *timer)
    {
        if (!timer)
            return NotAsked;
        else if (timer->isActive())
            return Asked;
        else
            return Yes;
    }

private:
    QPointer<QTimer> m_cancelConnectingTimer;
    QPointer<QTimer> m_cancelLockingDownTimer;
};

class PackageManager
{
    Q_DECLARE_TR_FUNCTIONS(Sfdk::PackageManager)

public:
    class PackageInfo
    {
    public:
        enum SymbolicVersion {
            NoSymbolicVersion,
            Latest,
            EarlyAccess
        };

        QString name;
        QString displayName;
        bool installed;
        QStringList dependencies;
        SymbolicVersion symbolicVersion = NoSymbolicVersion;
    };

    bool fetchInfo(QList<PackageInfo> *info, std::function<bool(const QString &)> filter)
    {
        const bool includeAvailable = true;
        return fetchInfo(info, includeAvailable, filter);
    }

    bool fetchInfo(QList<PackageInfo> *info, bool includeAvailable,
            std::function<bool(const QString &)> filter)
    {
        // Avoid unnecessary network access - use an empty temporary repository unless
        // includeAvailable is set
        auto maybeTempRepository = [=]() -> std::unique_ptr<QTemporaryDir> {
            if (includeAvailable)
                return {};

            if (qEnvironmentVariableIsSet(Constants::NO_TEMP_REPOSITORY_ENV_VAR))
                return {};

            auto dir = std::make_unique<QTemporaryDir>(QDir::temp().filePath("sfdk-empty-repo"));
            QTC_ASSERT(dir->isValid(), return {});

            FileSaver updatesXml(dir->filePath("Updates.xml"), QIODevice::WriteOnly);
            updatesXml.write(MINIMAL_UPDATES_XML);
            const bool ok = updatesXml.finalize();
            // If Updates.xml is missing or corrupted, maintenance tool should
            // complain but still do the expected job, so do not fail here.
            QTC_ASSERT(ok, qCWarning(sfdk) << updatesXml.errorString());

            return dir;
        }();

        QProcess listPackages;
        listPackages.setProgram(SdkManager::sdkMaintenanceToolPath());
        listPackages.setProcessEnvironment(addQpaPlatformMinimal());

        QStringList arguments{"--verbose", "--manage-packages", "non-interactive=1",
            "accept-licenses=1", "list-packages=1"};
        if (maybeTempRepository) {
            const QUrl url = QUrl::fromLocalFile(maybeTempRepository->path());
            arguments += {"--setTempRepository", url.toString()};
        }
        listPackages.setArguments(arguments);

        listPackages.start();
        if (!listPackages.waitForFinished(-1)
                || listPackages.exitStatus() != QProcess::NormalExit
                || (listPackages.exitCode() != EXIT_SUCCESS
                    && listPackages.exitCode() != SDK_MAINTENANCE_TOOL_EXIT_CANCEL)) {
            qerr() << tr("Failed to list installer-provided packages");
            const QByteArray stdErr = listPackages.readAllStandardError();
            if (!stdErr.isEmpty())
                qerr() << ": " << stdErr;
            qerr() << endl;
            return false;
        }

        QTextStream listing(listPackages.readAllStandardOutput());

        // "SailfishOS-latest-i486 (1.2.3)" -> "SailfishOS-1.2.3-i486" + "latest"
        auto parseDisplayName = [](const QString &displayName, QString *name,
                PackageInfo::SymbolicVersion *symbolicVersion) {
            QRegularExpression re(R"(^(\S+)-(ea|latest)(-\S+)? \((\S+)\)$)");
            QRegularExpressionMatch match = re.match(displayName);
            if (match.hasMatch()) {
                const QString symbolicVersionString = match.captured(2);
                *name = match.captured(1) + '-' + match.captured(4);
                if (symbolicVersionString == "ea") {
                    *symbolicVersion = PackageInfo::EarlyAccess;
                    *name += "EA";
                } else {
                    *symbolicVersion = PackageInfo::Latest;
                }
                *name += match.captured(3);
            } else {
                *name = displayName;
                *symbolicVersion = PackageInfo::NoSymbolicVersion;
            }
        };

        QRegularExpression re(R"re(^.*list-packages:: (available|installed) (\S+) "(.+)" requires (\S+))re");
        QString line;
        while (listing.readLineInto(&line)) {
            QRegularExpressionMatch match = re.match(line);
            if (!match.hasMatch())
                continue;

            PackageInfo i;

            i.name = match.captured(2);
            if (!filter(i.name))
                continue;

            i.displayName = match.captured(3);
            parseDisplayName(match.captured(3), &i.displayName, &i.symbolicVersion);

            i.installed = match.captured(1) == "installed";
            i.dependencies = match.captured(4).split(',');

            *info << i;
        }

        return true;
    }

    bool installPackage(const QString &name)
    {
        QList<PackageInfo> info;
        bool fetchOk = fetchInfo(&info, [name](const QString &packageName) {
            return packageName == name;
        });
        if (!fetchOk)
            return false;

        QTC_ASSERT(!info.isEmpty(), return false);
        QTC_ASSERT(!info.first().installed, return false);

        QProcess managePackages;
        managePackages.setProgram(SdkManager::sdkMaintenanceToolPath());
        managePackages.setArguments({"--verbose", "--manage-packages",
                "non-interactive=1", "accept-licenses=1", "add-packages=" + name});
        managePackages.setProcessEnvironment(addQpaPlatformMinimal());
        managePackages.setProcessChannelMode(QProcess::ForwardedChannels);
        managePackages.start();
        if (!managePackages.waitForFinished(-1)
                || managePackages.exitStatus() != QProcess::NormalExit
                || managePackages.exitCode() != EXIT_SUCCESS) {
            qerr() << tr("Failed to install installer-provided packages");
            const QByteArray stdErr = managePackages.readAllStandardError();
            if (!stdErr.isEmpty())
                qerr() << ": " << stdErr;
            qerr() << endl;
            return false;
        }

        return true;
    }

    bool removePackage(const QString &name)
    {
        QList<PackageInfo> info;
        bool fetchOk = fetchInfo(&info, [name](const QString &packageName) {
            return packageName == name;
        });
        if (!fetchOk)
            return false;

        QTC_ASSERT(!info.isEmpty(), return false);
        QTC_ASSERT(info.first().installed, return false);

        QProcess managePackages;
        managePackages.setProgram(SdkManager::sdkMaintenanceToolPath());
        managePackages.setArguments({"--verbose", "--manage-packages",
                "non-interactive=1", "accept-licenses=1", "remove-packages=" + name});
        managePackages.setProcessEnvironment(addQpaPlatformMinimal());
        managePackages.setProcessChannelMode(QProcess::ForwardedChannels);
        managePackages.start();
        if (!managePackages.waitForFinished(-1)
                || managePackages.exitStatus() != QProcess::NormalExit
                || managePackages.exitCode() != EXIT_SUCCESS) {
            qerr() << tr("Failed to uninstall installer-provided packages");
            const QByteArray stdErr = managePackages.readAllStandardError();
            if (!stdErr.isEmpty())
                qerr() << ": " << stdErr;
            qerr() << endl;
            return false;
        }

        return true;
    }
};

class ToolsPackageManager
{
    Q_DECLARE_TR_FUNCTIONS(Sfdk::ToolsPackageManager)

public:
    bool listTools(SdkManager::ListToolsOptions options, QList<ToolsInfo> *info)
    {
        QTC_ASSERT(options & SdkManager::InstalledTools, return false); // Cannot exclude these
        QTC_ASSERT(info, return false);

        if (!fetchInstallerProvidedToolsInfo(options & SdkManager::AvailableTools))
            return false;

        if (options & SdkManager::UserDefinedTools) {
            if (!fetchCustomToolingsInfo())
                return false;
            if (!fetchCustomTargetsInfo(options & SdkManager::CheckSnapshots))
                return false;
        }

        for (auto it = m_toolingFlagsByPackage.cbegin(); it != m_toolingFlagsByPackage.cend(); ++it) {
            if (!validate(*it))
                qCWarning(sfdk) << "Invalid flags for tooling" << it.key() << *it;
        }

        for (auto it = m_targetFlagsByPackage.cbegin(); it != m_targetFlagsByPackage.cend(); ++it) {
            if (!validate(*it))
                qCWarning(sfdk) << "Invalid flags for target" << it.key() << *it;
        }

        QList<ToolsInfo> toolingInfo;
        QList<ToolsInfo> targetInfo;
        QList<ToolsInfo> snapshotInfo;

        for (const QString &name : m_toolingPackageByName.keys()) {
            const QString package = m_toolingPackageByName.value(name);
            const ToolsInfo::Flags flags = m_toolingFlagsByPackage.value(package);
#if defined(Q_CC_MSVC)
            ToolsInfo i;
            i.name = name;
            i.flags = flags;
            toolingInfo << i;
#else
            toolingInfo << ToolsInfo{name, {}, flags};
#endif
        }

        for (const QString &name : m_targetPackageByName.keys()) {
            const QString package = m_targetPackageByName.value(name);
            if (m_snapshotSourceNameByTargetPackage.contains(package)) {
                if (!(options & SdkManager::SnapshotTools))
                    continue;
                const QString sourceTargetName = m_snapshotSourceNameByTargetPackage.value(package);
                const ToolsInfo::Flags flags = m_targetFlagsByPackage.value(package);
#if defined(Q_CC_MSVC)
                ToolsInfo i;
                i.name = name;
                i.parentName = sourceTargetName;
                i.flags = flags;
                snapshotInfo << i;
#else
                snapshotInfo << ToolsInfo{name, sourceTargetName, flags};
#endif
            } else {
                const QString toolingName =
                    m_toolingNameByPackage.value(m_toolingPackageByTargetPackage.value(package));
                const ToolsInfo::Flags flags = m_targetFlagsByPackage.value(package);
#if defined(Q_CC_MSVC)
                ToolsInfo i;
                i.name = name;
                i.parentName = toolingName;
                i.flags = flags;
                targetInfo << i;
#else
                targetInfo << ToolsInfo{name, toolingName, flags};
#endif
            }
        }

        *info = toolingInfo + targetInfo + snapshotInfo;
        return true;
    }

    bool updateTools(const QString &name, SdkManager::ToolsTypeHint typeHint)
    {
        QStringList args;
        args += toArgs(typeHint);
        args += "update";
        args += name;

        const int exitCode = SdkManager::runOnEngine("sdk-assistant", args);
        return exitCode == EXIT_SUCCESS;
    }

    bool registerTools(const QString &maybeName, SdkManager::ToolsTypeHint typeHint,
            const QString &maybeUserName, const QString &maybePassword)
    {
        QStringList args;
        args += toArgs(typeHint);
        args += "register";
        if (!maybeName.isEmpty())
            args += maybeName;
        else
            args += "--all";
        if (!maybeUserName.isEmpty())
            args += {"--user", maybeUserName};
        if (!maybePassword.isEmpty())
            args += {"--password", maybePassword};

        const int exitCode = SdkManager::runOnEngine("sdk-assistant", args);
        return exitCode == EXIT_SUCCESS;
    }

    bool installTools(const QString &name, SdkManager::ToolsTypeHint typeHint)
    {
        if (!fetchInfo())
            return false;

        QString package;
        ToolsInfo::Flags flags;
        if (!packageForName(name, typeHint, &package, &flags))
            return false;

        if (flags & ToolsInfo::UserDefined) {
            qerr() << tr("Name used by user-defined tools: \"%1\"").arg(name) << endl;
            return false;
        }

        if (flags & ToolsInfo::Installed) {
            qerr() << tr("Already installed: \"%1\"").arg(name) << endl;
            return false;
        }

        if (!(flags & ToolsInfo::Available)) {
            // Snapshot name used?
            qerr() << tr("Invalid name: \"%1\"").arg(name) << endl;
            return false;
        }

        return PackageManager().installPackage(package);
    }

    bool installCustomTools(const QString &name, const QString &imageFileOrUrl,
            SdkManager::ToolsTypeHint typeHint, const QString &maybeTooling)
    {
        QStringList args;
        args += toArgs(typeHint);
        args += "create";
        args += name;
        args += imageFileOrUrl;
        if (!maybeTooling.isEmpty()) {
            args += "--tooling";
            args += maybeTooling;
        }
        const int exitCode = SdkManager::runOnEngine("sdk-assistant", args);
        return exitCode == EXIT_SUCCESS;
    }

    bool removeTools(const QString &name, SdkManager::ToolsTypeHint typeHint, bool snapshotsOf)
    {
        if (!fetchInfo())
            return false;

        QString package;
        ToolsInfo::Flags flags;
        if (!packageForName(name, typeHint, &package, &flags))
            return false;

        if (flags & ToolsInfo::UserDefined || flags & ToolsInfo::Snapshot || snapshotsOf) {
            QStringList args;
            args += toArgs(typeHint);
            args += "remove";
            if (snapshotsOf)
                args += "--snapshots-of";
            args += name;
            const int exitCode = SdkManager::runOnEngine("sdk-assistant", args);
            return exitCode == EXIT_SUCCESS;
        }

        if (!(flags & ToolsInfo::Installed)) {
            qerr() << tr("Not installed: \"%1\"").arg(name) << endl;
            return false;
        }

        return PackageManager().removePackage(package);
    }

private:
    bool fetchInfo(bool checkSnapshots = false)
    {
        if (!fetchInstallerProvidedToolsInfo())
            return false;
        if (!fetchCustomToolingsInfo())
            return false;
        if (!fetchCustomTargetsInfo(checkSnapshots))
            return false;

        return true;
    }

    bool fetchInstallerProvidedToolsInfo(bool includeAvailable = true)
    {
        QList<PackageManager::PackageInfo> allPackageInfo;
        const bool ok = PackageManager().fetchInfo(&allPackageInfo, includeAvailable,
                [](const QString &package) {
            return package.startsWith("org.merproject.targets.")
                && package != "org.merproject.targets.toolings"
                && package != "org.merproject.targets.user"
                && !package.startsWith("org.merproject.targets.user.")
                && !package.endsWith("4qtcreator");
        });
        if (!ok)
            return false;

        for (const PackageManager::PackageInfo &info : allPackageInfo) {
            ToolsInfo::Flags flags;

            if (!info.installed && !includeAvailable)
                continue;

            if (info.installed)
                flags |= ToolsInfo::Installed;
            else
                flags |= ToolsInfo::Available;

            if (info.symbolicVersion == PackageManager::PackageInfo::Latest)
                flags |= ToolsInfo::Latest;
            else if (info.symbolicVersion == PackageManager::PackageInfo::EarlyAccess)
                flags |= ToolsInfo::EarlyAccess;

            if (info.name.startsWith("org.merproject.targets.toolings.")) {
                m_toolingPackageByName.insert(info.displayName, info.name);
                m_toolingNameByPackage.insert(info.name, info.displayName);
                m_toolingFlagsByPackage.insert(info.name, flags | ToolsInfo::Tooling);
            } else {
                m_targetPackageByName.insert(info.displayName, info.name);
                m_targetNameByPackage.insert(info.name, info.displayName);
                m_targetFlagsByPackage.insert(info.name, flags | ToolsInfo::Target);

                QRegularExpression toolingRe(R"(^org\.merproject\.targets\.toolings\.)");
                QStringList requiredToolings = info.dependencies.filter(toolingRe);
                QTC_ASSERT(requiredToolings.count() == 1, continue);
                m_toolingPackageByTargetPackage.insert(info.name, requiredToolings.first());
            }
        }

        return true;
    }

    bool fetchCustomToolingsInfo()
    {
        QBuffer out;
        out.open(QIODevice::ReadWrite);

        QStringList listToolingsArgs = {"tooling", "list", "--long"};
        const bool runInTerminal = false;
        const int exitCode = SdkManager::runOnEngine("sdk-manage", listToolingsArgs, {}, runInTerminal,
                &out);
        if (exitCode != EXIT_SUCCESS)
            return false;

        out.seek(0);

        QTextStream stream(&out);
        QString line;
        while (stream.readLineInto(&line)) {
            const QStringList splitted = line.split(' ', Qt::SkipEmptyParts);
            QTC_ASSERT(splitted.count() == 2, continue);
            const QString name = splitted.at(0);
            const QString mode = splitted.at(1);

            if (mode != "user")
                continue;

            const QString package = CUSTOM_PACKAGES_PREFIX + name;
            m_toolingPackageByName.insert(name, package);
            m_toolingNameByPackage.insert(package, name);
            m_toolingFlagsByPackage.insert(package,
                    ToolsInfo::Flags(ToolsInfo::Tooling) | ToolsInfo::UserDefined);
        }

        return true;
    }

    bool fetchCustomTargetsInfo(bool checkSnapshots)
    {
        QBuffer out;
        out.open(QIODevice::ReadWrite);

        QStringList args = {"target", "list", "--long"};
        if (checkSnapshots)
            args += "--check-snapshots";
        const bool runInTerminal = false;
        const int exitCode = SdkManager::runOnEngine("sdk-manage", args, {}, runInTerminal, &out);
        if (exitCode != EXIT_SUCCESS)
            return false;

        out.seek(0);

        QTextStream stream(&out);
        QString line;
        while (stream.readLineInto(&line)) {
            const QStringList splitted = line.split(' ', Qt::SkipEmptyParts);
            QTC_ASSERT(splitted.count() == 4, continue);
            const QString name = splitted.at(0);
            const QString tooling = splitted.at(1);
            const QString mode = splitted.at(2);
            const QString snapshotInfo = splitted.at(3);

            if (mode != "user")
                continue;

            const QString package = CUSTOM_PACKAGES_PREFIX + name;

            m_targetPackageByName.insert(name, package);
            m_targetNameByPackage.insert(package, name);
            m_toolingPackageByTargetPackage.insert(package, m_toolingPackageByName.value(tooling));

            if (snapshotInfo != "-") {
                QString sourceTargetName;
                ToolsInfo::Flag snapshotFlag;
                parseSnapshotInfo(snapshotInfo, &sourceTargetName, &snapshotFlag);

                m_snapshotSourceNameByTargetPackage.insert(package, sourceTargetName);
                m_targetFlagsByPackage.insert(package,
                        ToolsInfo::Flags(ToolsInfo::Target) | ToolsInfo::Snapshot | snapshotFlag);
            } else {
                m_targetFlagsByPackage.insert(package,
                        ToolsInfo::Flags(ToolsInfo::Target) | ToolsInfo::UserDefined);
            }
        }

        return true;
    }

    bool packageForName(const QString &name, SdkManager::ToolsTypeHint typeHint, QString *package,
            ToolsInfo::Flags *flags) const
    {
        switch (typeHint) {
        case SdkManager::NoToolsHint:
            *package = m_toolingPackageByName.value(name);
            if (!package->isEmpty()) {
                if (m_targetPackageByName.contains(name)) {
                    qerr() << tr("Name \"%1\" matches both a tooling and a target. Choose one.")
                        .arg(name) << endl;
                    return false;
                }
                *flags = m_toolingFlagsByPackage.value(*package);
            } else {
                *package = m_targetPackageByName.value(name);
                *flags = m_targetFlagsByPackage.value(*package);
            }
            break;
        case SdkManager::ToolingHint:
            *package = m_toolingPackageByName.value(name);
            if (package->isEmpty()) {
                qerr() << tr("No such tooling: \"%1\".").arg(name) << endl;
                return false;
            }
            *flags = m_toolingFlagsByPackage.value(*package);
            break;
        case SdkManager::TargetHint:
            *package = m_targetPackageByName.value(name);
            if (package->isEmpty()) {
                qerr() << tr("No such target: \"%1\".").arg(name) << endl;
                return false;
            }
            *flags = m_targetFlagsByPackage.value(*package);
            break;
        }

        return true;
    }

    static void parseSnapshotInfo(const QString &snapshotInfo, QString *sourceTargetName,
            ToolsInfo::Flag *snapshotFlag)
    {
        *sourceTargetName = snapshotInfo;
        *snapshotFlag = ToolsInfo::NoFlag;
        if (sourceTargetName->endsWith('*')) {
            sourceTargetName->chop(1);
            *snapshotFlag = ToolsInfo::Outdated;
        }
    }

    static QStringList toArgs(SdkManager::ToolsTypeHint typeHint)
    {
        switch (typeHint) {
        case SdkManager::NoToolsHint:
            return {};
        case SdkManager::ToolingHint:
            return {"tooling"};
        case SdkManager::TargetHint:
            return {"target"};
        }
        QTC_CHECK(false);
        return {};
    }

    static bool validate(ToolsInfo::Flags flags)
    {
        using I = ToolsInfo;

        constexpr int maxFlags = std::numeric_limits<
            std::underlying_type_t<I::Flags::enum_type>
            >::digits;
        static_assert(maxFlags > 0, "Limits error");
        using BitSet = std::bitset<maxFlags>;

        QTC_ASSERT((flags & I::Tooling) != (flags & I::Target), return false);
        QTC_ASSERT(BitSet(flags & (I::Available|I::Installed|I::UserDefined|I::Snapshot))
                .count() <= 1, return false);
        QTC_ASSERT(!(flags & I::Snapshot) || (flags & I::Target), return false);
        QTC_ASSERT(!(flags & I::Outdated) || (flags & I::Snapshot), return false);
        QTC_ASSERT(BitSet(flags & (I::Latest|I::EarlyAccess)).count() <= 1, return false);
        QTC_ASSERT(!(flags & (I::Latest|I::EarlyAccess)) || (flags & (I::Available|I::Installed)),
                return false);

        return true;
    }

private:
    QHash<QString, QString> m_toolingPackageByName;
    QHash<QString, QString> m_toolingNameByPackage;
    QHash<QString, ToolsInfo::Flags> m_toolingFlagsByPackage;

    QHash<QString, QString> m_targetPackageByName;
    QHash<QString, QString> m_targetNameByPackage;
    QHash<QString, ToolsInfo::Flags> m_targetFlagsByPackage;
    QHash<QString, QString> m_toolingPackageByTargetPackage;
    QHash<QString, QString> m_snapshotSourceNameByTargetPackage;
};

class EmulatorPackageManager
{
    Q_DECLARE_TR_FUNCTIONS(Sfdk::EmulatorPackageManager)

public:
    bool listEmulators(SdkManager::ListEmulatorsOptions options, QList<EmulatorInfo> *info)
    {
        QTC_ASSERT(options & SdkManager::InstalledEmulators, return false); // Cannot exclude these
        QTC_ASSERT(info, return false);

        if (!fetchInstallerProvidedEmulatorsInfo(options & SdkManager::AvailableEmulators))
            return false;

        if (options & SdkManager::UserDefinedEmulators) {
            if (!fetchCustomEmulatorsInfo())
                return false;
        }

        for (auto it = m_flagsByPackage.cbegin(); it != m_flagsByPackage.cend(); ++it) {
            if (!validate(*it))
                qCWarning(sfdk) << "Invalid flags for emulator" << it.key() << *it;
        }

        for (const QString &name : m_packageByName.keys()) {
            const QString package = m_packageByName.value(name);
            const EmulatorInfo::Flags flags = m_flagsByPackage.value(package);
#if defined(Q_CC_MSVC)
            EmulatorInfo i;
            i.name = name;
            i.flags = flags;
            *info << i;
#else
            *info << EmulatorInfo{name, flags};
#endif
        }

        return true;
    }

    bool installEmulator(const QString &name)
    {
        if (!fetchInfo())
            return false;

        QString package;
        EmulatorInfo::Flags flags;
        if (!packageForName(name, &package, &flags))
            return false;

        if (flags & EmulatorInfo::UserDefined) {
            qerr() << tr("Name used by user-defined emulator: \"%1\"").arg(name) << endl;
            return false;
        }

        if (flags & EmulatorInfo::Installed) {
            qerr() << tr("Already installed: \"%1\"").arg(name) << endl;
            return false;
        }

        return PackageManager().installPackage(package);
    }

    bool removeEmulator(const QString &name)
    {
        if (!fetchInfo())
            return false;

        QString package;
        EmulatorInfo::Flags flags;
        if (!packageForName(name, &package, &flags))
            return false;

        if (flags & EmulatorInfo::UserDefined) {
            // FIXME Unimplemented
            qerr() << tr("Cannot remove user-defined emulator \"%1\"") << endl;
            return false;
        }

        if (!(flags & EmulatorInfo::Installed)) {
            qerr() << tr("Not installed: \"%1\"").arg(name) << endl;
            return false;
        }

        return PackageManager().removePackage(package);
    }

private:
    bool fetchInfo()
    {
        if (!fetchInstallerProvidedEmulatorsInfo())
            return false;
        if (!fetchCustomEmulatorsInfo())
            return false;

        return true;
    }

    bool fetchInstallerProvidedEmulatorsInfo(bool includeAvailable = true)
    {
        QList<PackageManager::PackageInfo> allPackageInfo;
        const bool ok = PackageManager().fetchInfo(&allPackageInfo, includeAvailable,
                [](const QString &package) {
            return package.startsWith("org.merproject.emulators.")
                && !package.contains("4qtcreator_");
        });
        if (!ok)
            return false;

        Emulator *const defaultEmulator = Sdk::emulators().isEmpty()
            ? nullptr
            : Sdk::emulators().first();

        for (const PackageManager::PackageInfo &info : allPackageInfo) {
            EmulatorInfo::Flags flags;

            if (info.installed)
                flags |= EmulatorInfo::Installed;
            else
                flags |= EmulatorInfo::Available;

            if (info.symbolicVersion == PackageManager::PackageInfo::Latest)
                flags |= EmulatorInfo::Latest;
            else if (info.symbolicVersion == PackageManager::PackageInfo::EarlyAccess)
                flags |= EmulatorInfo::EarlyAccess;

            if (defaultEmulator && info.displayName == defaultEmulator->name())
                flags |= EmulatorInfo::Default;

            m_packageByName.insert(info.displayName, info.name);
            m_nameByPackage.insert(info.name, info.displayName);
            m_flagsByPackage.insert(info.name, flags);
        }

        return true;
    }

    bool fetchCustomEmulatorsInfo()
    {
        Emulator *const defaultEmulator = Sdk::emulators().isEmpty()
            ? nullptr
            : Sdk::emulators().first();

        for (Emulator *const emulator : Sdk::emulators()) {
            if (emulator->isAutodetected())
                continue;

            EmulatorInfo::Flags flags = EmulatorInfo::UserDefined;

            if (emulator == defaultEmulator)
                flags |= EmulatorInfo::Default;

            const QString package = CUSTOM_PACKAGES_PREFIX + emulator->name();
            m_packageByName.insert(emulator->name(), package);
            m_nameByPackage.insert(package, emulator->name());
            m_flagsByPackage.insert(package, flags);
        }

        return true;
    }

    bool packageForName(const QString &name, QString *package, EmulatorInfo::Flags *flags) const
    {
        *package = m_packageByName.value(name);
        if (package->isEmpty()) {
            qerr() << tr("No such emulator: \"%1\".").arg(name) << endl;
            return false;
        }
        *flags = m_flagsByPackage.value(*package);

        return true;
    }

    static bool validate(EmulatorInfo::Flags flags)
    {
        using I = EmulatorInfo;

        constexpr int maxFlags = std::numeric_limits<
            std::underlying_type_t<I::Flags::enum_type>
            >::digits;
        static_assert(maxFlags > 0, "Limits error");
        using BitSet = std::bitset<maxFlags>;

        QTC_ASSERT(BitSet(flags & (I::Available|I::Installed)).count() == 1, return false);
        QTC_ASSERT(BitSet(flags & (I::Latest|I::EarlyAccess)).count() <= 1, return false);

        return true;
    }

private:
    QHash<QString, QString> m_packageByName;
    QHash<QString, QString> m_nameByPackage;
    QHash<QString, EmulatorInfo::Flags> m_flagsByPackage;
};

} // namespace Sfdk

/*!
 * \class SdkManager
 */

SdkManager *SdkManager::s_instance = nullptr;

SdkManager::SdkManager(bool useSystemSettingsOnly)
{
    Q_ASSERT(!s_instance);
    s_instance = this;

    if (!qEnvironmentVariableIsEmpty(Constants::DISABLE_REVERSE_PATH_MAPPING_ENV_VAR))
        setEnableReversePathMapping(false);

    m_merSettings = std::make_unique<MerSettings>();

    BuildEngine::registerVmConnectionUi<VmConnectionUi>();
    Emulator::registerVmConnectionUi<VmConnectionUi>();

    Sdk::Options sdkOptions = Sdk::NoOption;
    if (qEnvironmentVariableIsEmpty(Constants::DISABLE_VM_INFO_CACHE_ENV_VAR))
        sdkOptions |= Sdk::CachedVmInfo;
    if (useSystemSettingsOnly) {
        sdkOptions &= ~Sdk::CachedVmInfo;
        sdkOptions |= Sdk::SystemSettingsOnly;
    }
    m_sdk = std::make_unique<Sdk>(sdkOptions);

    QList<BuildEngine *> buildEngines = Sdk::buildEngines();
    if (!buildEngines.isEmpty()) {
        m_buildEngine = buildEngines.first();
        if (buildEngines.count() > 1) {
            qCWarning(sfdk) << "Multiple build engines found. Using"
                << m_buildEngine->uri().toString();
        }
    }
}

SdkManager::~SdkManager()
{
    execAsynchronous(std::tie(), Sdk::shutDown);
    s_instance = nullptr;
}

QString SdkManager::installationPath()
{
    return Sdk::installationPath();
}

QString SdkManager::sdkMaintenanceToolPath()
{
    return Sdk::installationPath() + '/' + SDK_MAINTENANCE_TOOL;
}

bool SdkManager::hasEngine()
{
    return s_instance->m_buildEngine != nullptr;
}

QString SdkManager::noEngineFoundMessage()
{
    return tr("No build engine found");
}

BuildEngine *SdkManager::engine()
{
    QTC_ASSERT(s_instance->hasEngine(), return nullptr);
    return s_instance->m_buildEngine;
}

bool SdkManager::startEngine()
{
    QTC_ASSERT(s_instance->hasEngine(), return false);
    return startReliably(s_instance->m_buildEngine->virtualMachine());
}

bool SdkManager::stopEngine()
{
    QTC_ASSERT(s_instance->hasEngine(), return false);
    return stopReliably(s_instance->m_buildEngine->virtualMachine());
}

bool SdkManager::isEngineRunning()
{
    QTC_ASSERT(s_instance->hasEngine(), return false);
    return isRunningReliably(s_instance->m_buildEngine->virtualMachine());
}

int SdkManager::runOnEngine(const QString &program, const QStringList &arguments,
        const QProcessEnvironment &extraEnvironment_,
        Utils::optional<bool> runInTerminal, QIODevice *out, QIODevice *err)
{
    QTC_ASSERT(s_instance->hasEngine(), return SFDK_EXIT_ABNORMAL);

    // Assumption to minimize the time spent here: if the VM is running, we must have been waiting
    // before for the engine to fully start, so no need to wait for connectTo again
    if (!isEngineRunning()) {
        qCInfo(sfdk).noquote() << tr("Starting the build engine…");
        bool ok;
        execAsynchronous(std::tie(ok), std::mem_fn(&VirtualMachine::connectTo),
                s_instance->m_buildEngine->virtualMachine(), VirtualMachine::NoConnectOption);
        if (!ok) {
            qerr() << tr("Failed to start the build engine") << endl;
            return SFDK_EXIT_ABNORMAL;
        }
    }

    DBusManager::Ptr dbus = DBusManager::get(s_instance->m_buildEngine->dBusPort(),
            Constants::BUILD_ENGINE_SYSTEM_BUS_CONNECTION);

    const QProcessEnvironment systemEnvironment = QProcessEnvironment::systemEnvironment();

    QString program_ = program;
    QStringList arguments_ = arguments;
    QString workingDirectory = QDir::current().canonicalPath();

    QProcessEnvironment extraEnvironment = s_instance->environmentToForwardToEngine();
    extraEnvironment.insert(extraEnvironment_);
    extraEnvironment.insert(Mer::Constants::SAILFISH_SDK_FRONTEND,
            systemEnvironment.value(Mer::Constants::SAILFISH_SDK_FRONTEND,
                Constants::SDK_FRONTEND_ID));
    if (dbus)
        extraEnvironment.insert(Constants::SAILFISH_SDK_SFDK_DBUS_SERVICE, dbus->serviceName());

    if (!s_instance->mapEnginePaths(&program_, &arguments_, &workingDirectory, &extraEnvironment))
        return SFDK_EXIT_ABNORMAL;

    std::unique_ptr<QFile> stdOut;
    if (!out) {
        stdOut = binaryOut(stdout);
        out = stdOut.get();
    }

    std::unique_ptr<QFile> stdErr;
    if (!err) {
        stdErr = binaryOut(stderr);
        err = stdErr.get();
    }

    RemoteProcess process;
    process.setSshParameters(s_instance->m_buildEngine->virtualMachine()->sshParameters());
    process.setProgram(program_);
    process.setArguments(arguments_);
    process.setWorkingDirectory(workingDirectory);
    process.setExtraEnvironment(extraEnvironment);
    process.setRunInTerminal(runInTerminal);
    process.setInputChannelMode(QProcess::ForwardedInputChannel);

    QObject::connect(&process, &RemoteProcess::standardOutput, [&](const QByteArray &data) {
        out->write(s_instance->maybeReverseMapEnginePaths(data));
        if (stdOut)
            stdOut->flush();
    });
    QObject::connect(&process, &RemoteProcess::standardError, [&](const QByteArray &data) {
        err->write(s_instance->maybeReverseMapEnginePaths(data));
        if (stdErr)
            stdErr->flush();
    });
    QObject::connect(&process, &RemoteProcess::connectionError, [&](const QString &errorString) {
        qerr() << tr("Error connecting to the build engine: ") << errorString << endl;
    });
    QObject::connect(&process, &RemoteProcess::processError, [&](const QString &errorString) {
        qerr() << tr("Error running command on the build engine: ") << errorString << endl;
    });

    return process.exec();
}

int SdkManager::runHook(const QString &program, const QStringList &arguments,
        const QString &workingDirectory, const QProcessEnvironment &extraEnvironment)
{
    QString program_ = program;
    QStringList arguments_ = arguments;
    QString workingDirectory_ = workingDirectory;
    QProcessEnvironment extraEnvironment_ = extraEnvironment;

    if (!s_instance->reverseMapEnginePaths(&program_, &arguments_, &workingDirectory_, &extraEnvironment_))
        return EXIT_FAILURE;

    return runHookNative(program_, arguments_, workingDirectory_, extraEnvironment_);
}

int SdkManager::runHookNative(const QString &program, const QStringList &arguments,
        const QString &workingDirectory, const QProcessEnvironment &extraEnvironment)
{
    qCDebug(sfdk) << "About to run hook" << program;

    QProcess hook;
    hook.setProgram(program);
    hook.setArguments(arguments);
    hook.setWorkingDirectory(workingDirectory);
    hook.setProcessChannelMode(QProcess::ForwardedChannels);

    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    environment.insert(extraEnvironment);
    hook.setProcessEnvironment(environment);

    hook.start();
    if (!hook.waitForFinished() || hook.exitStatus() != QProcess::NormalExit) {
        qerr() << tr("Error running hook \"%1\"").arg(program) << endl;
        return EXIT_FAILURE;
    }

    qCDebug(sfdk) << "Hook exited with code" << hook.exitCode();

    return hook.exitCode();
}

int SdkManager::runHookNativeByName(const QString &hook, const QStringList &arguments,
        const QString &workingDirectory, const QProcessEnvironment &extraEnvironment)
{
    const Option *const hooksDirOption_ = Dispatcher::option(Constants::HOOKS_DIR_OPTION_NAME);
    QTC_ASSERT(hooksDirOption_, return EXIT_SUCCESS);

    const Utils::optional<OptionEffectiveOccurence> hooksDirOption =
        Configuration::effectiveState(hooksDirOption_);
    if (!hooksDirOption)
        return EXIT_SUCCESS;
    QTC_ASSERT(!hooksDirOption->isMasked(), return EXIT_SUCCESS);

    const FilePath hooksDir = FilePath::fromString(hooksDirOption->argument());
    if (!hooksDir.exists())
        return EXIT_SUCCESS;

    const FilePath program = hooksDir.pathAppended(hook);
    if (!program.exists())
        return EXIT_SUCCESS;

    return runHookNative(program.toString(), arguments, workingDirectory, extraEnvironment);
}

void SdkManager::setEnableReversePathMapping(bool enable)
{
    // Enabled by default, not meant to be flipped temporarily!
    QTC_ASSERT(!enable, return);

    if (s_instance->m_enableReversePathMapping)
        qCDebug(sfdk) << "Disabling reverse path mapping";

    s_instance->m_enableReversePathMapping = enable;
}

bool SdkManager::listTools(ListToolsOptions options, QList<ToolsInfo> *info)
{
    QTC_ASSERT(s_instance->hasEngine(), return false);
    return ToolsPackageManager().listTools(options, info);
}

bool SdkManager::updateTools(const QString &name, ToolsTypeHint typeHint)
{
    QTC_ASSERT(s_instance->hasEngine(), return false);
    return ToolsPackageManager().updateTools(name, typeHint);
}

bool SdkManager::registerTools(const QString &maybeName, ToolsTypeHint typeHint,
        const QString &maybeUserName, const QString &maybePassword)
{
    QTC_ASSERT(s_instance->hasEngine(), return false);
    return ToolsPackageManager().registerTools(maybeName, typeHint, maybeUserName, maybePassword);
}

bool SdkManager::installTools(const QString &name, ToolsTypeHint typeHint)
{
    QTC_ASSERT(s_instance->hasEngine(), return false);
    return ToolsPackageManager().installTools(name, typeHint);
}

bool SdkManager::installCustomTools(const QString &name, const QString &imageFileOrUrl,
        ToolsTypeHint typeHint, const QString &maybeTooling)
{
    QTC_ASSERT(s_instance->hasEngine(), return false);
    return ToolsPackageManager().installCustomTools(name, imageFileOrUrl, typeHint, maybeTooling);
}

bool SdkManager::removeTools(const QString &name, ToolsTypeHint typeHint, bool snapshotsOf)
{
    QTC_ASSERT(s_instance->hasEngine(), return false);
    return ToolsPackageManager().removeTools(name, typeHint, snapshotsOf);
}

BuildTargetData SdkManager::configuredTarget(QString *errorMessage)
{
    Q_ASSERT(errorMessage);
    QTC_ASSERT(s_instance->hasEngine(), return {});

    const Option *const targetOption_ = Dispatcher::option(Constants::TARGET_OPTION_NAME);
    QTC_ASSERT(targetOption_, return {});
    const Utils::optional<OptionEffectiveOccurence> targetOption =
        Configuration::effectiveState(targetOption_);

    const Option *const snapshotOption_ = Dispatcher::option(Constants::SNAPSHOT_OPTION_NAME);
    QTC_ASSERT(snapshotOption_, return {});
    const Utils::optional<OptionEffectiveOccurence> snapshotOption =
        Configuration::effectiveState(snapshotOption_);

    const Option *const noSnapshotOption_ = Dispatcher::option(Constants::NO_SNAPSHOT_OPTION_NAME);
    QTC_ASSERT(noSnapshotOption_, return {});
    const Utils::optional<OptionEffectiveOccurence> noSnapshotOption =
        Configuration::effectiveState(noSnapshotOption_);

    if (!targetOption) {
        *errorMessage = tr("No target selected");
        return {};
    }

    BuildTargetData retv;

    if (noSnapshotOption) {
        retv = engine()->buildTarget(targetOption->argument());
    } else if (!snapshotOption) {
        retv = engine()->buildTargetByOrigin(targetOption->argument());
    } else if (!snapshotOption->argument().startsWith("%pool")) {
        retv = engine()->buildTargetByOrigin(targetOption->argument(),
                snapshotOption->argument());
    } else {
        FileReader reader;
        if (!reader.fetch(TARGET_NAME_FILE)) {
            if (QFileInfo::exists(TARGET_NAME_FILE))
                qCWarning(sfdk).noquote() << reader.errorString();
            *errorMessage = tr("Feature not available with temporary snapshots before first build step is executed");
            return {};
        }
        const QString targetName = QString::fromUtf8(reader.data());
        QTC_CHECK(!targetName.isEmpty());
        retv = engine()->buildTarget(targetName);
    }

    // Target not synchronized to host
    if (!retv.isValid())
        *errorMessage = tr("Feature not available with the selected target");

    return retv;
}

Device *SdkManager::configuredDevice(QString *errorMessage)
{
    Q_ASSERT(errorMessage);

    const Option *const deviceOption_ = Dispatcher::option(Constants::DEVICE_OPTION_NAME);
    QTC_ASSERT(deviceOption_, return nullptr);

    const Utils::optional<OptionEffectiveOccurence> deviceOption =
        Configuration::effectiveState(deviceOption_);
    if (!deviceOption) {
        *errorMessage = tr("No device selected");
        return nullptr;
    }
    QTC_ASSERT(!deviceOption->isMasked(), return nullptr);

    return deviceByName(deviceOption->argument(), errorMessage);
}

Device *SdkManager::deviceByName(const QString &deviceName, QString *errorMessage)
{
    Q_ASSERT(errorMessage);

    Device *device = nullptr;
    for (Device *const device_ : Sdk::devices()) {
        if (device_->name() == deviceName) {
            if (device) {
                *errorMessage = tr("Ambiguous device name \"%1\". Please fix your configuration.")
                    .arg(deviceName);
                return nullptr;
            }
            device = device_;
        }
    }

    if (!device)
        *errorMessage = deviceName + ": " + tr("No such device");

    return device;
}

bool SdkManager::prepareForRunOnDevice(const Device &device, RemoteProcess *process)
{
    // Keep in sync with mb2
    if (runHookNativeByName("prepare-device", {device.name()}, QDir::currentPath(),
                QProcessEnvironment()) != EXIT_SUCCESS) {
        qerr() << tr("The \"prepare-device\" hook failed") << endl;
        return false;
    }

    if (device.machineType() == Device::EmulatorMachine) {
        Emulator *const emulator = static_cast<const EmulatorDevice &>(device).emulator();

        // Assumption to minimize the time spent here: if the VM is running, we must have been waiting
        // before for the emulator to fully start, so no need to wait for connectTo again
        if (!isEmulatorRunning(*emulator)) {
            qCInfo(sfdk).noquote() << tr("Starting the emulator…");
            bool ok;
            execAsynchronous(std::tie(ok), std::mem_fn(&VirtualMachine::connectTo),
                    emulator->virtualMachine(), VirtualMachine::NoConnectOption);
            if (!ok) {
                qerr() << tr("Failed to start the emulator") << endl;
                return false;
            }
        }
    }

    process->setSshParameters(device.sshParameters());

    QObject::connect(process, &RemoteProcess::connectionError, [&](const QString &errorString) {
        if (device.machineType() == Device::EmulatorMachine)
            qerr() << tr("Error connecting to the emulator: ") << errorString << endl;
        else
            qerr() << tr("Error connecting to the device: ") << errorString << endl;
    });
    QObject::connect(process, &RemoteProcess::processError, [&](const QString &errorString) {
        if (device.machineType() == Device::EmulatorMachine)
            qerr() << tr("Error running command on the emulator: ") << errorString << endl;
        else
            qerr() << tr("Error running command on the device: ") << errorString << endl;
    });

    return true;
}

int SdkManager::runOnDevice(const Device &device, const QString &program,
        const QStringList &arguments, Utils::optional<bool> runInTerminal)
{
    std::unique_ptr<QFile> stdOut = binaryOut(stdout);
    std::unique_ptr<QFile> stdErr = binaryOut(stderr);

    RemoteProcess process;
    process.setProgram(program);
    process.setArguments(arguments);
    process.setRunInTerminal(runInTerminal);
    process.setInputChannelMode(QProcess::ForwardedInputChannel);

    QObject::connect(&process, &RemoteProcess::standardOutput, [&](const QByteArray &data) {
        stdOut->write(data);
        stdOut->flush();
    });
    QObject::connect(&process, &RemoteProcess::standardError, [&](const QByteArray &data) {
        stdErr->write(data);
        stdErr->flush();
    });

    if (!prepareForRunOnDevice(device, &process))
        return SFDK_EXIT_ABNORMAL;

    return process.exec();
}

Emulator *SdkManager::emulatorByName(const QString &emulatorName, QString *errorMessage)
{
    Q_ASSERT(errorMessage);

    Emulator *emulator = nullptr;
    for (Emulator *const emulator_ : Sdk::emulators()) {
        if (emulator_->name() == emulatorName) {
            if (emulator) {
                *errorMessage = tr("Ambiguous emulator name \"%1\". Please fix your configuration.")
                    .arg(emulatorName);
                return nullptr;
            }
            emulator = emulator_;
        }
    }

    if (!emulator)
        *errorMessage = emulatorName + ": " + tr("No such emulator");

    return emulator;
}

bool SdkManager::startEmulator(const Emulator &emulator)
{
    return startReliably(emulator.virtualMachine());
}

bool SdkManager::stopEmulator(const Emulator &emulator)
{
    return stopReliably(emulator.virtualMachine());
}

bool SdkManager::isEmulatorRunning(const Emulator &emulator)
{
    return isRunningReliably(emulator.virtualMachine());
}

int SdkManager::runOnEmulator(const Emulator &emulator, const QString &program,
        const QStringList &arguments, Utils::optional<bool> runInTerminal)
{
    Device *const emulatorDevice = Sdk::device(emulator);
    QTC_ASSERT(emulatorDevice, return SFDK_EXIT_ABNORMAL);
    return runOnDevice(*emulatorDevice, program, arguments, runInTerminal);
}

bool SdkManager::listEmulators(ListEmulatorsOptions options, QList<EmulatorInfo> *info)
{
    return EmulatorPackageManager().listEmulators(options, info);
}

bool SdkManager::installEmulator(const QString &name)
{
    return EmulatorPackageManager().installEmulator(name);
}

bool SdkManager::removeEmulator(const QString &name)
{
    return EmulatorPackageManager().removeEmulator(name);
}

bool SdkManager::startReliably(VirtualMachine *virtualMachine)
{
    QTC_ASSERT(virtualMachine, return false);
    bool ok;
    if (virtualMachine->isLockedDown()) {
        execAsynchronous(std::tie(ok), std::mem_fn(&VirtualMachine::lockDown),
                virtualMachine, false);
    }
    execAsynchronous(std::tie(ok), std::mem_fn(&VirtualMachine::connectTo),
            virtualMachine, VirtualMachine::NoConnectOption);
    return ok; // NB, just the last call matters
}

bool SdkManager::stopReliably(VirtualMachine *virtualMachine)
{
    QTC_ASSERT(virtualMachine, return false);
    bool ok;
    if (isRunningReliably(virtualMachine)) {
        execAsynchronous(std::tie(ok), std::mem_fn(&VirtualMachine::connectTo),
                virtualMachine, VirtualMachine::NoConnectOption);
    }
    execAsynchronous(std::tie(ok), std::mem_fn(&VirtualMachine::lockDown),
            virtualMachine, true);
    return ok; // NB, just the last call matters
}

bool SdkManager::isRunningReliably(VirtualMachine *virtualMachine)
{
    QTC_ASSERT(virtualMachine, return false);
    bool ok;
    execAsynchronous(std::tie(ok), std::mem_fn(&VirtualMachine::refreshState),
            virtualMachine);
    return ok && !virtualMachine->isOff();
}

void SdkManager::saveSettings()
{
    QStringList errorStrings;
    s_instance->m_sdk->saveSettings(&errorStrings);
    for (const QString &errorString : errorStrings)
        qCWarning(sfdk).noquote() << "Error saving settings:" << errorString;
}

QString SdkManager::cleanSharedSrc() const
{
    QTC_ASSERT(hasEngine(), return {});
    return QDir(QDir::cleanPath(m_buildEngine->sharedSrcPath().toString())).canonicalPath();
}

QString SdkManager::cleanSharedTarget(QString *errorMessage) const
{
    Q_ASSERT(errorMessage);
    QTC_ASSERT(hasEngine(), return {});
    const BuildTargetData target = SdkManager::configuredTarget(errorMessage);
    if (!target.isValid())
        return {};

    return QDir(QDir::cleanPath(target.sysRoot.toString())).canonicalPath();
}

bool SdkManager::mapEnginePaths(QString *program, QStringList *arguments, QString *workingDirectory,
        QProcessEnvironment *environment) const
{
    QTC_ASSERT(hasEngine(), return false);
    *program = QDir::fromNativeSeparators(*program);

    QString errorString;
    const QString cleanSharedSrc = this->cleanSharedSrc();
    const QString cleanSharedTarget = this->cleanSharedTarget(&errorString);
    if (cleanSharedTarget.isEmpty())
        qCDebug(sfdk).noquote() << "Not mapping shared target path:" << errorString;

    if (!workingDirectory->startsWith(cleanSharedSrc)) {
        qCDebug(sfdk) << "cleanSharedSrc:" << cleanSharedSrc;
        qerr() << tr("The command needs to be used under %1 workspace, "
                "which is currently configured as \"%2\"")
            .arg(Sdk::sdkVariant())
            .arg(m_buildEngine->sharedSrcPath().toString()) << endl;
        return false;
    }

    Qt::CaseSensitivity caseInsensitiveOnWindows = Utils::HostOsInfo::isWindowsHost()
        ? Qt::CaseInsensitive
        : Qt::CaseSensitive;

    struct Mapping {
        QString hostPath;
        QString enginePath;
        Qt::CaseSensitivity cs;
    };
    std::vector<Mapping> mappings;
    if (!cleanSharedTarget.isEmpty()) {
        mappings.push_back({cleanSharedTarget + "/", "/", Qt::CaseSensitive});
        mappings.push_back({cleanSharedTarget, "/", Qt::CaseSensitive});
    }
    mappings.push_back({cleanSharedSrc, m_buildEngine->sharedSrcMountPoint(),
            caseInsensitiveOnWindows});

    for (const Mapping &mapping : mappings) {
        QTC_ASSERT(!mapping.hostPath.isEmpty(), continue);
        qCDebug(sfdk) << "Mapping" << mapping.hostPath << "as" << mapping.enginePath;
        auto mapGreedy = [=](QString string) {
            QRegularExpression::PatternOption reCaseSensitivity = mapping.cs == Qt::CaseInsensitive
                    ? QRegularExpression::CaseInsensitiveOption
                    : QRegularExpression::NoPatternOption;
            QRegularExpression re(QString("(?:%1|%2)([^:;,'\"]*)")
                                  .arg(QRegularExpression::escape(mapping.hostPath))
                                  .arg(QRegularExpression::escape(QDir::toNativeSeparators(mapping.hostPath))),
                                  reCaseSensitivity);
            // Replace longer matches first! Use map for sorting.
            std::map<QString, QString> replacements;
            QRegularExpressionMatchIterator reIterator = re.globalMatch(string);
            while (reIterator.hasNext()) {
                QRegularExpressionMatch match = reIterator.next();
                QString fullNativeHostPath = match.captured(0);
                QString fullEnginePath = QDir::fromNativeSeparators(mapping.enginePath + match.captured(1));
                replacements.insert(std::pair<QString, QString>(fullNativeHostPath, fullEnginePath));
            }
            for (auto i = replacements.crbegin(); i != replacements.crend(); ++i)
                string.replace(i->first, i->second);
            return string;
        };

        program->replace(mapping.hostPath, mapping.enginePath, mapping.cs);
        *arguments = Utils::transform(*arguments, mapGreedy);
        workingDirectory->replace(mapping.hostPath, mapping.enginePath, mapping.cs);
        for (const QString &key : environment->keys()) {
            QString value = environment->value(key);
            environment->insert(key, mapGreedy(value));
        }
    }

    qCDebug(sfdk) << "Command after mapping engine paths:" << *program << "arguments:" << *arguments
        << "CWD:" << *workingDirectory;

    return true;
}

bool SdkManager::reverseMapEnginePaths(QString *program, QStringList *arguments,
        QString *workingDirectory, QProcessEnvironment *environment) const
{
    QTC_ASSERT(hasEngine(), return false);

    const QString cleanSharedSrc = this->cleanSharedSrc();
    QTC_ASSERT(!cleanSharedSrc.isEmpty(), return false);
    QTC_ASSERT(!m_buildEngine->sharedSrcMountPoint().isEmpty(), return false);

    auto map = [=](const QString &s) {
        return QString(s).replace(m_buildEngine->sharedSrcMountPoint(), cleanSharedSrc);
    };

    *program = map(*program);
    *arguments = Utils::transform(*arguments, map);
    *workingDirectory = map(*workingDirectory);
    for (const QString &key : environment->keys())
        environment->insert(key, map(environment->value(key)));

    return true;
}

QByteArray SdkManager::maybeReverseMapEnginePaths(const QByteArray &commandOutput) const
{
    QTC_ASSERT(hasEngine(), return {});

    // Ensure output consistency
    static bool reversePathMappingEnabledBefore = m_enableReversePathMapping;
    QTC_CHECK(reversePathMappingEnabledBefore == m_enableReversePathMapping);

    if (!m_enableReversePathMapping)
        return commandOutput;

    const QString cleanSharedSrc = this->cleanSharedSrc();

    QByteArray retv = commandOutput;

    QTC_ASSERT(!cleanSharedSrc.isEmpty(), return retv);
    QTC_ASSERT(!m_buildEngine->sharedSrcMountPoint().isEmpty(), return retv);

    retv.replace(m_buildEngine->sharedSrcMountPoint().toUtf8(), cleanSharedSrc.toUtf8());

    return retv;
}

QProcessEnvironment SdkManager::environmentToForwardToEngine() const
{
    const QStringList patterns = MerSettings::environmentFilter()
        .split(QRegularExpression("[[:space:]]+"), Qt::SkipEmptyParts);
    if (patterns.isEmpty())
        return {};

    QStringList regExps;
    for (const QString &pattern : patterns) {
        const QString asRegExp = QRegularExpression::escape(pattern).replace("\\*", ".*");
        regExps.append(asRegExp);
    }
    const QRegularExpression filter("^(" + regExps.join("|") + ")$");

    const QProcessEnvironment systemEnvironment = QProcessEnvironment::systemEnvironment();

    QProcessEnvironment retv;

    QStringList keys = systemEnvironment.keys();
    keys.sort();

    for (const QString &key : qAsConst(keys)) {
        if (key == Mer::Constants::SAILFISH_SDK_ENVIRONMENT_FILTER)
            continue;
        if (filter.match(key).hasMatch()) {
            if (Log::sfdk().isDebugEnabled()) {
                if (retv.isEmpty())
                    qCDebug(sfdk) << "Environment to forward to build engine (subject to path mapping):";
                const QString indent = Sfdk::indent(1);
                qCDebug(sfdk).noquote().nospace() << indent << key << '='
                    << Utils::QtcProcess::quoteArgUnix(systemEnvironment.value(key));
            }
            retv.insert(key, systemEnvironment.value(key));
        }
    }

    return retv;
}

#include "sdkmanager.moc"
