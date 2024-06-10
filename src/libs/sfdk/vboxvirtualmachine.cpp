/****************************************************************************
**
** Copyright (C) 2012-2019 Jolla Ltd.
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

#include "vboxvirtualmachine_p.h"

#include "asynchronous_p.h"
#include "sdk_p.h"
#include "sfdkconstants.h"

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/portlist.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QDir>
#include <QQueue>
#include <QRegularExpression>
#include <QSettings>
#include <QStack>
#include <QSize>
#include <QTime>
#include <QTimer>
#include <QTimerEvent>
#include <QUuid>

using namespace Utils;

namespace Sfdk {

namespace {
const char VBOXMANAGE[] = "VBoxManage";
const char SAILFISH_SDK_SYSTEM_VBOXMANAGE[] = "SAILFISH_SDK_SYSTEM_VBOXMANAGE";
const char GUEST_PROPERTIES_PATTERN[] = "/SailfishSDK/*";
const char SWAP_SIZE_MB_GUEST_PROPERTY_NAME[] = "/SailfishSDK/VM/Swap/SizeMb";
const char ENV_GUEST_PROPERTY_TEMPLATE[] = "/SailfishSDK/ENV/%1";
const char SAILFISH_SDK_DEVICE_MODEL[] = "SailfishSDK/DeviceModel";
const char SAILFISH_SDK_ORIENTATION[] = "SailfishSDK/Orientation";
const char SAILFISH_SDK_SCALE[] = "SailfishSDK/Scale";
const char PORTRAIT[] = "portrait";
const char LANDSCAPE[] = "landscape";
const char QML_LIVE_NATPF_RULE_NAME_MATCH[] = "qmllive_";
const char FREE_PORT_NATPF_RULE_NAME_MATCH[] = "freeport_";
const char NATPF_RULE_TEMPLATE[] = "%1,tcp,127.0.0.1,%2,,%3";
const char FREE_PORT_NATPF_RULE_NAME_TEMPLATE[] = "freeport_%1";
const char QML_LIVE_NATPF_RULE_NAME_TEMPLATE[] = "qmllive_%1";
const char SSH_NATPF_RULE_NAME[] = "guestssh";
const char DBUS_NATPF_RULE_NAME[] = "guestdbus";
} // namespace anonymous

class VBoxVirtualMachineInfo : public VirtualMachineInfo
{
public:
    QString storageUuid;

    // StorageInfo
    QStringList allRelatedStorageUuids;
};

namespace {

QString vBoxManagePath()
{
    static QString path;
    if (!path.isNull()) {
        return path;
    }

    if (HostOsInfo::isWindowsHost()) {
        path = QString::fromLocal8Bit(qgetenv("VBOX_INSTALL_PATH"));
        if (path.isEmpty()) {
            // env var name for VirtualBox 4.3.12 changed to this
            path = QString::fromLocal8Bit(qgetenv("VBOX_MSI_INSTALL_PATH"));
            if (path.isEmpty()) {
                // Not found in environment? Look up registry.
                QSettings s(QLatin1String("HKEY_LOCAL_MACHINE\\SOFTWARE\\Oracle\\VirtualBox"),
                            QSettings::NativeFormat);
                path = s.value(QLatin1String("InstallDir")).toString();
                if (path.startsWith(QLatin1Char('"')) && path.endsWith(QLatin1Char('"')))
                    path = path.mid(1, path.length() - 2); // remove quotes
            }
        }

        if (!path.isEmpty())
            path.append(QDir::separator() + QLatin1String(VBOXMANAGE));
    } else {
        QStringList searchPaths = QProcessEnvironment::systemEnvironment()
            .value(QLatin1String("PATH")).split(QLatin1Char(':'));
        // VBox 5 installs here for compatibility with Mac OS X 10.11
        searchPaths.append(QLatin1String("/usr/local/bin"));

        foreach (const QString &searchPath, searchPaths) {
            QDir dir(searchPath);
            if (dir.exists(QLatin1String(VBOXMANAGE))) {
                path = dir.absoluteFilePath(QLatin1String(VBOXMANAGE));
                break;
            }
        }
    }

    return path;
}

class VBoxManageRunner : public ProcessRunner
{
    Q_OBJECT

public:
    explicit VBoxManageRunner(const QStringList &arguments, QObject *parent = 0)
        : ProcessRunner(path(), arguments, parent)
    {
        process()->setProcessEnvironment(environment());
    }

    // VBoxManage has the bad habit of showing progress on stderr with no way
    // to disable it.  While it is convenient to have progress reported on
    // lenghty tasks it also clashes with foreground command output, breaking
    // line wrapping and alignment in general.
    void suppressNoisyOutput()
    {
        if (Log::vms().isDebugEnabled())
            return;

        process()->setProcessChannelMode(QProcess::SeparateChannels);
    }

    static QString path()
    {
        static QString path;
        if (!path.isNull())
            return path;

        path = vBoxManagePath();

        if (!SdkPrivate::customVBoxManagePath().isEmpty())
            path = SdkPrivate::customVBoxManagePath();

        if (!path.isEmpty())
            qCDebug(vms) << "Using vboxmanage tool at" << path;
        else
            qCDebug(vms) << "vboxmanage tool not found";

        return path;
    }

private:
    static QProcessEnvironment environment()
    {
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();

        if (!SdkPrivate::customVBoxManagePath().isEmpty())
            env.insert(SAILFISH_SDK_SYSTEM_VBOXMANAGE, vBoxManagePath());

        return env;
    }
};

} // namespace anonymous

/*!
 * \class VBoxVirtualMachine
 */

VBoxVirtualMachine::VBoxVirtualMachine(const QString &name, VirtualMachine::Features featureMask,
        std::unique_ptr<ConnectionUi> &&connectionUi, QObject *parent)
    : VirtualMachine(std::make_unique<VBoxVirtualMachinePrivate>(this), staticType(),
            staticFeatures() & featureMask, name, std::move(connectionUi), parent)
{
    Q_D(VBoxVirtualMachine);
    d->setDisplayType(staticDisplayType());
}

VBoxVirtualMachine::~VBoxVirtualMachine()
{
}

bool VBoxVirtualMachine::isAvailable()
{
    return !VBoxManageRunner::path().isEmpty();
}

QString VBoxVirtualMachine::staticType()
{
    return QStringLiteral("VirtualBox");
}

QString VBoxVirtualMachine::staticDisplayType()
{
    return tr("VirtualBox");
}

VirtualMachine::Features VBoxVirtualMachine::staticFeatures()
{
    return VirtualMachine::LimitMemorySize | VirtualMachine::LimitCpuCount
        | VirtualMachine::GrowStorageSize | VirtualMachine::OptionalHeadless
        | VirtualMachine::Snapshots | VirtualMachine::SwapMemory
        | VirtualMachine::ReserveStorageSize;
}

void VBoxVirtualMachine::fetchRegisteredVirtualMachines(const QObject *context,
        const Functor<const QStringList &, bool> &functor)
{
    using D = VBoxVirtualMachinePrivate;
    Q_ASSERT(context);
    Q_ASSERT(functor);

    QStringList arguments;
    arguments.append("list");
    arguments.append("vms");

    auto *runner = BatchComposer::enqueue<VBoxManageRunner>(arguments);
    connect(runner, &VBoxManageRunner::done, context, [=](bool ok) {
        if (!ok) {
            functor({}, false);
            return;
        }
        QStringList vms = D::listedVirtualMachines(QString::fromLocal8Bit(
                runner->process()->readAllStandardOutput()));
        functor(vms, true);
    });
}

/*!
 * \class VBoxVirtualMachinePrivate
 * \internal
 */

void VBoxVirtualMachinePrivate::fetchInfo(VirtualMachineInfo::ExtraInfos extraInfo,
        const QObject *context, const Functor<const VirtualMachineInfo &, bool> &functor) const
{
    Q_Q(const VBoxVirtualMachine);
    Q_ASSERT(context);
    Q_ASSERT(functor);

    BatchComposer composer = BatchComposer::createBatch("VBoxVirtualMachinePrivate::fetchInfo");
    composer.batch()->setPropagateFailure(true);
    QObject::connect(composer.batch(), &CommandRunner::failure,
            context, std::bind(functor, VirtualMachineInfo{}, false));

    auto info = std::make_shared<VBoxVirtualMachineInfo>();

    QStringList arguments;
    arguments.append("showvminfo");
    arguments.append(q->name());
    arguments.append("--machinereadable");

    auto *runner = BatchComposer::enqueue<VBoxManageRunner>(arguments);
    connect(runner, &VBoxManageRunner::success, context, [=]() {
        // FIXME return by argument
        *info = virtualMachineInfoFromOutput(
                QString::fromLocal8Bit(runner->process()->readAllStandardOutput()));
    });

    QStringList pArguments;
    pArguments.append("guestproperty");
    pArguments.append("enumerate");
    pArguments.append(q->name());
    pArguments.append("--patterns");
    pArguments.append(QLatin1String(GUEST_PROPERTIES_PATTERN));

    auto *pRunner = BatchComposer::enqueue<VBoxManageRunner>(pArguments);
    connect(pRunner, &VBoxManageRunner::success, context, [=]() {
        propertyBasedInfoFromOutput(QString::fromLocal8Bit(pRunner->process()->readAllStandardOutput()),
                info.get());
    });

    if (extraInfo & VirtualMachineInfo::StorageInfo) {
        QStringList arguments;
        arguments.append("list");
        arguments.append("hdds");

        auto *runner = BatchComposer::enqueue<VBoxManageRunner>(arguments);
        connect(runner, &VBoxManageRunner::success, context, [=]() {
            storageInfoFromOutput(QString::fromLocal8Bit(runner->process()->readAllStandardOutput()),
                    info.get());
        });
    }

    if (extraInfo & VirtualMachineInfo::SnapshotInfo) {
        QStringList arguments;
        arguments.append("snapshot");
        arguments.append(q->name());
        arguments.append("list");
        arguments.append("--machinereadable");

        auto *runner = BatchComposer::enqueue<VBoxManageRunner>(arguments);
        runner->setExpectedExitCodes({0, 1});
        connect(runner, &VBoxManageRunner::success, context, [=]() {
            // Command exits with 1 when no snapshot exists
            if (runner->process()->exitCode() == 1)
                return;
            snapshotInfoFromOutput(QString::fromLocal8Bit(runner->process()->readAllStandardOutput()),
                    info.get());
        });
    }

    BatchComposer::enqueueCheckPoint(context, [=]() { functor(*info, true); });
}

void VBoxVirtualMachinePrivate::start(const QObject *context, const Functor<bool> &functor)
{
    Q_Q(VBoxVirtualMachine);
    Q_ASSERT(context);
    Q_ASSERT(functor);

    qCDebug(vms) << "Starting" << q->uri().toString();

    QStringList arguments;
    arguments.append("startvm");
    arguments.append(q->name());
    if (q->isHeadless()) {
        arguments.append("--type");
        arguments.append("headless");
    }

    auto *runner = BatchComposer::enqueue<VBoxManageRunner>(arguments);
    connect(runner, &CommandRunner::done, context, functor);
}

void VBoxVirtualMachinePrivate::stop(const QObject *context, const Functor<bool> &functor)
{
    Q_Q(VBoxVirtualMachine);
    Q_ASSERT(context);
    Q_ASSERT(functor);

    qCDebug(vms) << "Stopping" << q->uri().toString();

    QStringList arguments;
    arguments.append("controlvm");
    arguments.append(q->name());
    arguments.append("acpipowerbutton");

    auto *runner = BatchComposer::enqueue<VBoxManageRunner>(arguments);
    connect(runner, &CommandRunner::done, context, functor);
}

void VBoxVirtualMachinePrivate::commit(const QObject *context, const Functor<bool> &functor)
{
    Q_Q(VBoxVirtualMachine);
    Q_ASSERT(context);
    Q_ASSERT(functor);

    qCDebug(vms) << "Committing (no-op)" << q->uri().toString();
    BatchComposer::enqueueCheckPoint(context, std::bind(functor, true));
}

void VBoxVirtualMachinePrivate::probe(const QObject *context,
        const Functor<BasicState, bool> &functor) const
{
    Q_Q(const VBoxVirtualMachine);
    Q_ASSERT(context);
    Q_ASSERT(functor);

    QStringList arguments;
    arguments.append("showvminfo");
    arguments.append(q->name());

    auto *runner = BatchComposer::enqueue<VBoxManageRunner>(arguments);
    connect(runner->process(), QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), context,
            [=](int exitCode, QProcess::ExitStatus exitStatus) {
                VirtualMachinePrivate::BasicState state;
                if (exitStatus != QProcess::NormalExit) {
                    functor(state, false);
                    return;
                }
                if (exitCode != 0) {
                    // Not critical to be very accurate here
                    functor(state, true);
                    return;
                }
                state |= VirtualMachinePrivate::Existing;

                bool isHeadless;
                bool isRunning = isVirtualMachineRunningFromInfo(
                    QString::fromLocal8Bit(runner->process()->readAllStandardOutput()), &isHeadless);
                if (isRunning)
                    state |= VirtualMachinePrivate::Running;
                if (isHeadless)
                    state |= VirtualMachinePrivate::Headless;
                functor(state, true);
            });
}

void VBoxVirtualMachinePrivate::setVideoMode(const QSize &size, int depth,
        const QString &deviceModelName, Qt::Orientation orientation, int scaleDownFactor,
        const QObject *context, const Functor<bool> &functor)
{
    Q_Q(VBoxVirtualMachine);
    Q_ASSERT(context);
    Q_ASSERT(functor);

    qCDebug(vms) << "Setting video mode for" << q->uri().toString() << "to size" << size
        << "depth" << depth;

    BatchComposer composer = BatchComposer::createBatch("VBoxVirtualMachinePrivate::setVideoMode");

    QString videoMode = QStringLiteral("%1x%2x%3")
        .arg(size.width())
        .arg(size.height())
        .arg(depth);
    QString hint = QStringLiteral("%1,%2")
        .arg(size.width())
        .arg(size.height());

    auto allOk = std::make_shared<bool>(true);
    auto enqueue = [=, name = q->name()](const QString &key, const QString &value) {
        setExtraData(key, value, Sdk::instance(), [=](bool ok) { *allOk &= ok; });
    };

    {
        BatchComposer composer = BatchComposer::createBatch("setLastNormalWindowPosition");
        fetchExtraData("GUI/LastNormalWindowPosition", Sdk::instance(),
                [=, batch = composer.batch()](const QString &data, bool ok) {
            *allOk &= ok;
            if (!ok)
                return;
            QTC_ASSERT(data.startsWith("Value:"), { *allOk = false; return; });
            const QStringList splitData = data.section(':',1,1).trimmed().split(',');
            QTC_ASSERT(splitData.size() > 1, { *allOk = false; return; });
            const QString lastPosition = splitData[0] + ',' + splitData[1] + ','
                + QString::number(size.width()) + ',' + QString::number(size.height());
            BatchComposer composer = BatchComposer::extendBatch(batch);
            enqueue("GUI/LastNormalWindowPosition", lastPosition);
        });
    }
    enqueue("CustomVideoMode1", videoMode);
    enqueue("GUI/LastGuestSizeHint", hint);
    enqueue("GUI/AutoresizeGuest", QLatin1Literal("false"));

    // These may be used by the optional VBoxManage wrapper
    enqueue(QLatin1String(SAILFISH_SDK_DEVICE_MODEL), deviceModelName);
    enqueue(QLatin1String(SAILFISH_SDK_ORIENTATION), orientation == Qt::Horizontal
            ? QLatin1String(LANDSCAPE)
            : QLatin1String(PORTRAIT));
    enqueue(QLatin1String(SAILFISH_SDK_SCALE), QString::number(scaleDownFactor));

    BatchComposer::enqueueCheckPoint(context, [=]() { functor(*allOk); });
}

void VBoxVirtualMachinePrivate::doInitGuest()
{
    // Do not check for "running", it's likely it will end in "degraded" state
    const QString watcher(R"(
        while [[ $(systemctl is-system-running) == starting ]]; do
            sleep 1
        done
    )");

    BatchComposer::enqueue<RemoteProcessRunner>("wait-for-startup-completion",
            watcher, q_func()->sshParameters());
}

void VBoxVirtualMachinePrivate::doSetMemorySizeMb(int memorySizeMb, const QObject *context,
        const Functor<bool> &functor)
{
    Q_Q(VBoxVirtualMachine);
    Q_ASSERT(context);
    Q_ASSERT(functor);

    qCDebug(vms) << "Changing memory size of" << q->uri().toString() << "to" << memorySizeMb << "MB";

    QStringList arguments;
    arguments.append("modifyvm");
    arguments.append(q->name());
    arguments.append("--memory");
    arguments.append(QString::number(memorySizeMb));

    auto *runner = BatchComposer::enqueue<VBoxManageRunner>(arguments);
    connect(runner, &CommandRunner::done, context, functor);
}

void VBoxVirtualMachinePrivate::doSetSwapSizeMb(int swapSizeMb, const QObject *context,
        const Functor<bool> &functor)
{
    Q_Q(VBoxVirtualMachine);
    Q_ASSERT(context);
    Q_ASSERT(functor);

    qCDebug(vms) << "Changing swap size of" << q->uri().toString() << "to" << swapSizeMb << "MB";

    QStringList arguments;
    arguments.append("guestproperty");
    arguments.append("set");
    arguments.append(q->name());
    arguments.append(QLatin1String(SWAP_SIZE_MB_GUEST_PROPERTY_NAME));
    arguments.append(QString::number(swapSizeMb));

    auto *runner = BatchComposer::enqueue<VBoxManageRunner>(arguments);
    connect(runner, &CommandRunner::done, context, functor);
}

void VBoxVirtualMachinePrivate::doSetCpuCount(int cpuCount, const QObject *context,
        const Functor<bool> &functor)
{
    Q_Q(VBoxVirtualMachine);
    Q_ASSERT(context);
    Q_ASSERT(functor);

    qCDebug(vms) << "Changing CPU count of" << q->uri().toString() << "to" << cpuCount;

    QStringList arguments;
    arguments.append("modifyvm");
    arguments.append(q->name());
    arguments.append("--cpus");
    arguments.append(QString::number(cpuCount));

    auto *runner = BatchComposer::enqueue<VBoxManageRunner>(arguments);
    connect(runner, &CommandRunner::done, context, functor);
}

void VBoxVirtualMachinePrivate::doSetStorageSizeMb(int storageSizeMb, const QObject *context,
        const Functor<bool> &functor)
{
    Q_Q(VBoxVirtualMachine);
    Q_ASSERT(context);
    Q_ASSERT(functor);

    qCDebug(vms) << "Changing storage size of" << q->uri().toString() << "to" << storageSizeMb << "MB";

    BatchComposer composer = BatchComposer::createBatch("VBoxVirtualMachinePrivate::doSetStorageSizeMb");

    const QPointer<const QObject> context_{context};
    fetchInfo(VirtualMachineInfo::StorageInfo, Sdk::instance(),
            [=, name = q->name(), batch = composer.batch()](const VirtualMachineInfo &virtualMachineInfo_, bool ok) {
        if (!ok) {
            callIf(context_, functor, false);
            return;
        }

        auto virtualMachineInfo = static_cast<const VBoxVirtualMachineInfo &>(virtualMachineInfo_);

        if (storageSizeMb < virtualMachineInfo.storageSizeMb) {
            qWarning() << "VBoxManage failed to resize medium" << virtualMachineInfo.storageUuid
                       << "to size [MB]" << storageSizeMb << "for VM" << name << ":"
                       << "Can't reduce storage size. Current size:" << virtualMachineInfo.storageSizeMb;
            callIf(context_, functor, false);
            return;
        } else if (storageSizeMb == virtualMachineInfo.storageSizeMb) {
            callIf(context_, functor, true);
            return;
        }

        if (virtualMachineInfo.allRelatedStorageUuids.isEmpty()) {
            // Something went wrong, an error message should have been already issued
            QTC_CHECK(false);
            callIf(context_, functor, false);
            return;
        }

        // Order matters!
        const QStringList toResize = virtualMachineInfo.allRelatedStorageUuids;
        qCDebug(vms) << "About to resize these storage images (in order):" << toResize;

        BatchComposer composer = BatchComposer::extendBatch(batch);

        auto allOk = std::make_shared<bool>(true);

        for (const QString &storageUuid : toResize) {
            QStringList arguments;
            arguments.append("modifymedium");
            arguments.append(storageUuid);
            arguments.append("--resize");
            arguments.append(QString::number(storageSizeMb));

            auto *runner = BatchComposer::enqueue<VBoxManageRunner>(arguments);
            QObject::connect(runner, &VBoxManageRunner::done, Sdk::instance(), [=](bool ok) {
                if (!ok) {
                    qWarning() << "VBoxManage failed to resize medium" << storageUuid;
                    *allOk = false;
                }
            });
        }

        BatchComposer::enqueueCheckPoint(context_.data(), [=]() { functor(*allOk); });
    });
}

void VBoxVirtualMachinePrivate::doSetSharedPath(SharedPath which, const FilePath &path,
        const QObject *context, const Functor<bool> &functor)
{
    Q_Q(VBoxVirtualMachine);
    Q_ASSERT(context);
    Q_ASSERT(functor);

    qCDebug(vms) << "Setting shared folder" << which << "for" << q->uri().toString() << "to" << path;

    QString mountName;
    QString alignedMountPoint;
    switch (which) {
    case VirtualMachinePrivate::SharedInstall:
        mountName = "install";
        break;
    case VirtualMachinePrivate::SharedConfig:
        mountName = "config";
        break;
    case VirtualMachinePrivate::SharedSsh:
        mountName = "ssh";
        break;
    case VirtualMachinePrivate::SharedHome:
        mountName = "home";
        break;
    case VirtualMachinePrivate::SharedTargets:
        mountName = "targets";
        break;
    case VirtualMachinePrivate::SharedSrc:
        mountName = "src1";
        alignedMountPoint = alignedMountPointFor(path.toString());
        break;
    case VirtualMachinePrivate::SharedMedia:
        mountName = "media";
        break;
    }

    BatchComposer composer = BatchComposer::createBatch("VBoxVirtualMachinePrivate::doSetSharedPath");
    composer.batch()->setPropagateFailure(true);
    connect(composer.batch(), &CommandRunner::done, context, functor);

    QStringList rargs;
    rargs.append("sharedfolder");
    rargs.append("remove");
    rargs.append(q->name());
    rargs.append("--name");
    rargs.append(mountName);
    BatchComposer::enqueue<VBoxManageRunner>(rargs);

    QStringList aargs;
    aargs.append("sharedfolder");
    aargs.append("add");
    aargs.append(q->name());
    aargs.append("--name");
    aargs.append(mountName);
    aargs.append("--hostpath");
    aargs.append(path.toString());
    BatchComposer::enqueue<VBoxManageRunner>(aargs);

    QStringList sargs;
    sargs.append("setextradata");
    sargs.append(q->name());
    sargs.append(QString("VBoxInternal2/SharedFoldersEnableSymlinksCreate/%1").arg(mountName));
    sargs.append("1");
    BatchComposer::enqueue<VBoxManageRunner>(sargs);

    if (!alignedMountPoint.isEmpty()) {
        const QString envName = QString::fromLatin1(Constants::BUILD_ENGINE_ALIGNED_MOUNT_POINT_ENV_TEMPLATE)
            .arg(mountName.toUpper());
        const QString envPropertyName = QString::fromLatin1(ENV_GUEST_PROPERTY_TEMPLATE).arg(envName);

        QStringList args;
        args.append("guestproperty");
        args.append("set");
        args.append(q->name());
        args.append(envPropertyName);
        args.append(alignedMountPoint);
        BatchComposer::enqueue<VBoxManageRunner>(args);
    }
}

void VBoxVirtualMachinePrivate::doAddPortForwarding(const QString &ruleName,
        const QString &protocol, quint16 hostPort, quint16 emulatorVmPort,
        const QObject *context, const Functor<bool> &functor)
{
    Q_Q(VBoxVirtualMachine);
    Q_ASSERT(context);
    Q_ASSERT(functor);

    qCDebug(vms) << "Setting port forwarding for" << q->uri().toString() << "from"
        << hostPort << "to" << emulatorVmPort;

    BatchComposer composer = BatchComposer::createBatch("VBoxVirtualMachinePrivate::doAddPortForwarding");

    QStringList dArguments;
    dArguments.append("modifyvm");
    dArguments.append(q->name());
    dArguments.append("--natpf1");
    dArguments.append("delete");
    dArguments.append(ruleName);

    BatchComposer::enqueue<VBoxManageRunner>(dArguments); // ignore failure

    QStringList arguments;
    arguments.append("modifyvm");
    arguments.append(q->name());
    arguments.append("--natpf1");
    arguments.append(QString::fromLatin1("%1,%2,,%3,,%4")
                     .arg(ruleName).arg(protocol).arg(hostPort).arg(emulatorVmPort));

    auto *runner = BatchComposer::enqueue<VBoxManageRunner>(arguments);
    connect(runner, &VBoxManageRunner::done, context, functor);
}

void VBoxVirtualMachinePrivate::doRemovePortForwarding(const QString &ruleName,
        const QObject *context, const Functor<bool> &functor)
{
    Q_Q(VBoxVirtualMachine);
    Q_ASSERT(context);
    Q_ASSERT(functor);

    qCDebug(vms) << "Deleting port forwarding rule" << ruleName << "from" << q->uri().toString();

    QStringList arguments;
    arguments.append("modifyvm");
    arguments.append(q->name());
    arguments.append("--natpf1");
    arguments.append("delete");
    arguments.append(ruleName);

    auto *runner = BatchComposer::enqueue<VBoxManageRunner>(arguments);
    connect(runner, &CommandRunner::done, context, functor);
}

void VBoxVirtualMachinePrivate::doSetReservedPortForwarding(ReservedPort which, quint16 port,
        const QObject *context, const Functor<bool> &functor)
{
    Q_Q(VBoxVirtualMachine);
    Q_ASSERT(context);
    Q_ASSERT(functor);

    qCDebug(vms) << "Setting reserved port forwarding" << which << "for" << q->uri().toString()
        << "to" << port;

    QString ruleName;
    int guestPort{};
    switch (which) {
        case VirtualMachinePrivate::SshPort:
            ruleName = QLatin1String(SSH_NATPF_RULE_NAME);
            guestPort = 22;
            break;
        case VirtualMachinePrivate::DBusPort:
            ruleName = QLatin1String(DBUS_NATPF_RULE_NAME);
            guestPort = 777;
            break;
    }
    const QString rule = QString::fromLatin1(NATPF_RULE_TEMPLATE)
        .arg(ruleName).arg(port).arg(guestPort);

    QStringList arguments;
    arguments.append("modifyvm");
    arguments.append(q->name());
    arguments.append("--natpf1");
    arguments.append("delete");
    arguments.append(ruleName);
    arguments.append("--natpf1");
    arguments.append(rule);

    auto *runner = BatchComposer::enqueue<VBoxManageRunner>(arguments);
    connect(runner, &CommandRunner::done, context, functor);
}

void VBoxVirtualMachinePrivate::doSetReservedPortListForwarding(ReservedPortList which,
        const QList<Utils::Port> &ports, const QObject *context,
        const Functor<const QMap<QString, quint16> &, bool> &functor)
{
    Q_Q(VBoxVirtualMachine);
    Q_ASSERT(context);
    Q_ASSERT(functor);
    QTC_CHECK(ports.count() <= Constants::MAX_PORT_LIST_PORTS);

    qCDebug(vms) << "Setting reserved port forwarding" << which << "for" << q->uri().toString()
        << "to" << ports;

    BatchComposer composer = BatchComposer::createBatch("VBoxVirtualMachinePrivate::doSetReservedPortListForwarding");

    QString ruleNameTemplate;
    QMap<QString, quint16> VirtualMachineInfo::*ruleMap = nullptr;
    switch (which) {
    case VirtualMachinePrivate::FreePortList:
        ruleNameTemplate = QLatin1String(FREE_PORT_NATPF_RULE_NAME_TEMPLATE);
        ruleMap = &VirtualMachineInfo::freePorts;
        break;
    case VirtualMachinePrivate::QmlLivePortList:
        ruleNameTemplate = QLatin1String(QML_LIVE_NATPF_RULE_NAME_TEMPLATE);
        ruleMap = &VirtualMachineInfo::qmlLivePorts;
        break;
    }
    const QString ruleTemplate = QString::fromLatin1(NATPF_RULE_TEMPLATE).arg(ruleNameTemplate);

    {
        BatchComposer composer = BatchComposer::createBatch("removeExisting");

        fetchInfo(VirtualMachineInfo::NoExtraInfo, Sdk::instance(),
                [=, batch = composer.batch()](const VirtualMachineInfo &info, bool ok) {
            QTC_ASSERT(ok, return);

            BatchComposer composer = BatchComposer::extendBatch(batch);

            const QStringList rulesToDelete = (info.*ruleMap).keys();
            for (const QString &ruleToDelete : rulesToDelete) {
                QStringList arguments;
                arguments.append("modifyvm");
                arguments.append(q->name());
                arguments.append("--natpf1");
                arguments.append("delete");
                arguments.append(ruleToDelete);

                auto *runner = BatchComposer::enqueue<VBoxManageRunner>(arguments);
                QObject::connect(runner, &VBoxManageRunner::failure, [=]() {
                    qWarning() << "VBoxManage failed to delete port forwarding rule" << ruleToDelete;
                });
            }
        });
    }

    auto savedPorts = std::make_shared<QMap<QString, quint16>>();

    QList<quint16> ports_ = Utils::transform(ports, &Port::number);
    std::sort(ports_.begin(), ports_.end());

    int i = 1;
    foreach (quint16 port, ports_) {
        QStringList arguments;
        arguments.append("modifyvm");
        arguments.append(q->name());
        arguments.append("--natpf1");
        arguments.append(ruleTemplate.arg(i).arg(port).arg(port));

        auto *runner = BatchComposer::enqueue<VBoxManageRunner>(arguments);
        QObject::connect(runner, &VBoxManageRunner::done, Sdk::instance(), [=](bool ok) {
            if (ok)
                savedPorts->insert(ruleNameTemplate.arg(i), port);
            else
                qWarning() << "VBoxManage failed to set" << which << "port" << port;
        });
        ++i;
    }

    BatchComposer::enqueueCheckPoint(context, [=]() {
        functor(*savedPorts, savedPorts->values() == ports_);
    });
}

void VBoxVirtualMachinePrivate::doTakeSnapshot(const QString &snapshotName, const QObject *context,
        const Functor<bool> &functor)
{
    Q_Q(VBoxVirtualMachine);
    Q_ASSERT(context);
    Q_ASSERT(functor);

    qCDebug(vms) << "Taking snapshot" << snapshotName << "of" << q->uri().toString();

    QStringList arguments;
    arguments.append("snapshot");
    arguments.append(q->name());
    arguments.append("take");
    arguments.append(snapshotName);

    auto *runner = BatchComposer::enqueue<VBoxManageRunner>(arguments);
    runner->suppressNoisyOutput();
    connect(runner, &CommandRunner::done, context, functor);
}

void VBoxVirtualMachinePrivate::doRestoreSnapshot(const QString &snapshotName, const QObject *context,
        const Functor<bool> &functor)
{
    Q_Q(VBoxVirtualMachine);
    Q_ASSERT(context);
    Q_ASSERT(functor);

    qCDebug(vms) << "Restoring snapshot" << snapshotName << "of" << q->uri().toString();

    QStringList arguments;
    arguments.append("snapshot");
    arguments.append(q->name());
    arguments.append("restore");
    arguments.append(snapshotName);

    auto *runner = BatchComposer::enqueue<VBoxManageRunner>(arguments);
    runner->suppressNoisyOutput();
    connect(runner, &CommandRunner::done, context, functor);
}

void VBoxVirtualMachinePrivate::doRemoveSnapshot(const QString &snapshotName, const QObject *context,
        const Functor<bool> &functor)
{
    Q_Q(VBoxVirtualMachine);
    Q_ASSERT(context);
    Q_ASSERT(functor);

    qCDebug(vms) << "Removing snapshot" << snapshotName << "of" << q->uri().toString();

    QStringList arguments;
    arguments.append("snapshot");
    arguments.append(q->name());
    arguments.append("delete");
    arguments.append(snapshotName);

    auto *runner = BatchComposer::enqueue<VBoxManageRunner>(arguments);
    runner->suppressNoisyOutput();
    connect(runner, &CommandRunner::done, context, functor);
}

void VBoxVirtualMachinePrivate::fetchExtraData(const QString &key,
        const QObject *context, const Functor<QString, bool> &functor) const
{
    Q_Q(const VBoxVirtualMachine);
    Q_ASSERT(context);
    Q_ASSERT(functor);

    QStringList arguments;
    arguments.append("getextradata");
    arguments.append(q->name());
    arguments.append(key);

    auto *runner = BatchComposer::enqueue<VBoxManageRunner>(arguments);
    connect(runner, &VBoxManageRunner::done, context, [=](bool ok) {
        if (!ok) {
            functor({}, false);
            return;
        }
        auto data = QString::fromLocal8Bit(runner->process()->readAllStandardOutput());
        functor(data, true);
    });
}

void VBoxVirtualMachinePrivate::setExtraData(const QString &keyword, const QString &data,
        const QObject *context, const Functor<bool> &functor)
{
    Q_Q(VBoxVirtualMachine);
    Q_ASSERT(context);
    Q_ASSERT(functor);

    QStringList args;
    args.append("setextradata");
    args.append(q->name());
    args.append(keyword);
    args.append(data);

    auto *runner = BatchComposer::enqueue<VBoxManageRunner>(args);
    connect(runner, &CommandRunner::done, context, functor);
}

bool VBoxVirtualMachinePrivate::isVirtualMachineRunningFromInfo(const QString &vmInfo, bool *headless)
{
    Q_ASSERT(headless);
    // VBox 4.x says "type", 5.x says "name"
    QRegularExpression re(QStringLiteral("^Session (name|type):\\s*(\\w+)"), QRegularExpression::MultilineOption);
    QRegularExpressionMatch match = re.match(vmInfo);
    *headless = match.captured(2) == QLatin1String("headless");
    return match.hasMatch();
}

QStringList VBoxVirtualMachinePrivate::listedVirtualMachines(const QString &output)
{
    QStringList vms;
    QRegExp rx(QLatin1String("\"(.*)\""));
    rx.setMinimal(true);
    int pos = 0;
    while ((pos = rx.indexIn(output, pos)) != -1) {
        pos += rx.matchedLength();
        if (rx.cap(1) != QLatin1String("<inaccessible>"))
            vms.append(rx.cap(1));
    }
    return vms;
}

VBoxVirtualMachineInfo VBoxVirtualMachinePrivate::virtualMachineInfoFromOutput(const QString &output)
{
    VBoxVirtualMachineInfo info;

    // Get ssh port, shared home and shared targets
    // 1 Name, 2 Protocol, 3 Host IP, 4 Host Port, 5 Guest IP, 6 Guest Port, 7 Shared Folder Name,
    // 8 Shared Folder Path 9 mac
    // 11 headed/headless (SessionType is for VBox 4.x, SessionName for VBox 5.x)
    // 12 SATA paths
    // 13 Memory size
    // 14 Count of CPU
    QRegExp rexp(QLatin1String("(?:Forwarding\\(\\d+\\)=\"(\\w+),(\\w+),(.*),(\\d+),(.*),(\\d+)\")"
                               "|(?:SharedFolderNameMachineMapping\\d+=\"(\\w+)\"\\W*"
                               "SharedFolderPathMachineMapping\\d+=\"(.*)\")"
                               "|(?:macaddress\\d+=\"(.*)\")"
                               "|(?:Session(Type|Name)=\"(.*)\")"
                               "|(?:\"SATA-ImageUUID-\\d-\\d\"=\"(.*)\")"
                               "|(?:memory=(\\d+)\\n)"
                               "|(?:cpus=(\\d+)\\n)"
                               ));

    rexp.setMinimal(true);
    int pos = 0;
    while ((pos = rexp.indexIn(output, pos)) != -1) {
        pos += rexp.matchedLength();
        if (rexp.cap(0).startsWith(QLatin1String("Forwarding"))) {
            QString ruleName = rexp.cap(1);
            quint16 port = rexp.cap(4).toUInt();
            if (ruleName.contains(QLatin1String(SSH_NATPF_RULE_NAME)))
                info.sshPort = port;
            else if (ruleName.contains(QLatin1String(DBUS_NATPF_RULE_NAME)))
                info.dBusPort = port;
            else if (ruleName.contains(QLatin1String(QML_LIVE_NATPF_RULE_NAME_MATCH)))
                info.qmlLivePorts[ruleName] = port;
            else if (ruleName.contains(QLatin1String(FREE_PORT_NATPF_RULE_NAME_MATCH)))
                info.freePorts[ruleName] = port;
            else
                info.otherPorts[ruleName] = port;
        } else if(rexp.cap(0).startsWith(QLatin1String("SharedFolderNameMachineMapping"))) {
            if (rexp.cap(7) == QLatin1String("install"))
                info.sharedInstall = QDir::cleanPath(rexp.cap(8));
            else if (rexp.cap(7) == QLatin1String("home"))
                info.sharedHome = QDir::cleanPath(rexp.cap(8));
            else if (rexp.cap(7) == QLatin1String("targets"))
                info.sharedTargets = QDir::cleanPath(rexp.cap(8));
            else if (rexp.cap(7) == QLatin1String("ssh"))
                info.sharedSsh = QDir::cleanPath(rexp.cap(8));
            else if (rexp.cap(7) == QLatin1String("config"))
                info.sharedConfig = QDir::cleanPath(rexp.cap(8));
            else if (rexp.cap(7).startsWith(QLatin1String("src")))
                info.sharedSrc = QDir::cleanPath(rexp.cap(8));
            else if (rexp.cap(7) == QLatin1String("media"))
                info.sharedMedia = QDir::cleanPath(rexp.cap(8));
        } else if(rexp.cap(0).startsWith(QLatin1String("macaddress"))) {
            QRegExp rx(QLatin1String("(?:([0-9A-F]{2})([0-9A-F]{2})([0-9A-F]{2})([0-9A-F]{2})([0-9A-F]{2})([0-9A-F]{2}))"));
            QString mac = rexp.cap(9);
            QStringList macFields;
            if(rx.exactMatch(mac)) {
                macFields = rx.capturedTexts();
            }
            if(!macFields.isEmpty()) {
                macFields.removeFirst();
                info.macs << macFields.join(QLatin1Char(':'));
            }
        } else if (rexp.cap(0).startsWith(QLatin1String("Session"))) {
            info.headless = rexp.cap(11) == QLatin1String("headless");
        } else if (rexp.cap(0).startsWith(QLatin1String("\"SATA-ImageUUID"))) {
            QString storageUuid = rexp.cap(12);
            if (!storageUuid.isEmpty() && storageUuid != QLatin1String("none")) {
                info.storageUuid = storageUuid;
            }
        } else if (rexp.cap(0).startsWith(QLatin1String("memory"))) {
            int memorySize = rexp.cap(13).toInt();
            if (memorySize > 0) {
                info.memorySizeMb = memorySize;
            }
        } else if (rexp.cap(0).startsWith(QLatin1String("cpus"))) {
            int cpu = rexp.cap(14).toInt();
            if (cpu > 0) {
                info.cpuCount = cpu;
            }
        }
    }

    return info;
}

void VBoxVirtualMachinePrivate::propertyBasedInfoFromOutput(const QString &output,
        VBoxVirtualMachineInfo *virtualMachineInfo)
{
    QRegExp rexp(QLatin1String("^Name: +([^ ]+), +value: ([^ ]+), "));

    for (const QString &line : output.split('\n')) {
        if (rexp.indexIn(line) != -1) {
            const QString name = rexp.cap(1);
            const QString value = rexp.cap(2);

            if (name == QLatin1String(SWAP_SIZE_MB_GUEST_PROPERTY_NAME)) {
                virtualMachineInfo->swapSizeMb = value.toInt();
            }
        }
    }
}

void VBoxVirtualMachinePrivate::storageInfoFromOutput(const QString &output,
        VBoxVirtualMachineInfo *virtualMachineInfo)
{
    // 1 UUID
    // 2 Parent UUID
    // 3 Storage capacity
    QRegExp rexp(QLatin1String("(?:UUID:\\s+([^\\s]+))"
                               "|(?:Parent UUID:\\s+([^\\s]+))"
                               "|(?:Capacity:\\s+(\\d+) MBytes)"
                               ));

    QHash<QString, QPair<QString, QStringList>> parentAndChildrenUuids;

    QString currentUuid;
    int pos = 0;
    while ((pos = rexp.indexIn(output, pos)) != -1) {
        pos += rexp.matchedLength();
        if (rexp.cap(0).startsWith(QLatin1String("UUID"))) {
            currentUuid = rexp.cap(1);
            QTC_ASSERT(!QUuid(currentUuid).isNull(), return);
        } else if (rexp.cap(0).startsWith(QLatin1String("Parent UUID"))) {
            QTC_ASSERT(!currentUuid.isNull(), return);
            QString parentUuid = rexp.cap(2);
            if (parentUuid == "base") {
                parentAndChildrenUuids[currentUuid].first = QString();
                continue;
            }
            QTC_ASSERT(!QUuid(parentUuid).isNull(), return);
            parentAndChildrenUuids[currentUuid].first = parentUuid;
            parentAndChildrenUuids[parentUuid].second << currentUuid;
        } else if (rexp.cap(0).startsWith(QLatin1String("Capacity"))) {
            if (currentUuid != virtualMachineInfo->storageUuid)
                continue;
            QString capacityStr = rexp.cap(3);
            QTC_ASSERT(!capacityStr.isEmpty(), continue);
            bool ok;
            int capacity = capacityStr.toInt(&ok);
            QTC_CHECK(ok);
            virtualMachineInfo->storageSizeMb = ok ? capacity : 0;
        }
    }

    QTC_ASSERT(parentAndChildrenUuids.contains(virtualMachineInfo->storageUuid), return);

    QSet<QString> relatedUuids;
    relatedUuids.insert(virtualMachineInfo->storageUuid);
    QStack<QString> uuidsToCheck;
    uuidsToCheck.push(virtualMachineInfo->storageUuid);
    while (!uuidsToCheck.isEmpty()) {
        QString uuid = uuidsToCheck.pop();
        auto parentAndChildrenUuidsIt = parentAndChildrenUuids.constFind(uuid);
        QTC_ASSERT(parentAndChildrenUuidsIt != parentAndChildrenUuids.end(), return);

        if (!parentAndChildrenUuidsIt->first.isNull()) {
            if (!relatedUuids.contains(parentAndChildrenUuidsIt->first)) {
                relatedUuids.insert(parentAndChildrenUuidsIt->first);
                uuidsToCheck.push(parentAndChildrenUuidsIt->first);
            }
        }
        for (const QString &childUuid : parentAndChildrenUuidsIt->second) {
            if (!relatedUuids.contains(childUuid)) {
                relatedUuids.insert(childUuid);
                uuidsToCheck.push(childUuid);
            }
        }
    }

    virtualMachineInfo->allRelatedStorageUuids = relatedUuids.values();
}

int VBoxVirtualMachinePrivate::ramSizeFromOutput(const QString &output, bool *matched)
{
    Q_ASSERT(matched);
    *matched = false;

    QRegExp rexp(QLatin1String("(RAM/Usage/Total\\s+(\\d+) kB)"));

    rexp.setMinimal(true);
    int pos = 0;
    while ((pos = rexp.indexIn(output, pos)) != -1) {
        pos += rexp.matchedLength();
        int memSize = rexp.cap(2).toInt();
        *matched = true;
        return memSize;
    }
    return 0;
}

void VBoxVirtualMachinePrivate::snapshotInfoFromOutput(const QString &output, VBoxVirtualMachineInfo *virtualMachineInfo)
{
    // 1 name
    QRegExp rexp(QLatin1String("\\bSnapshotName[-0-9]*=\"(.+)\""));

    rexp.setMinimal(true);
    int pos = 0;
    while ((pos = rexp.indexIn(output, pos)) != -1) {
        pos += rexp.matchedLength();
        virtualMachineInfo->snapshots.append(rexp.cap(1));
    }
}

} // namespace Sfdk

#include "vboxvirtualmachine.moc"
