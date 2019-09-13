/****************************************************************************
**
** Copyright (C) 2012-2019 Jolla Ltd.
** Copyright (C) 2019 Open Mobile Platform LLC.
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

#include "virtualboxmanager_p.h"

#include "asynchronous_p.h"
#include "sdk_p.h"
#include "sfdkconstants.h"

#include <utils/algorithm.h>
#include <utils/hostosinfo.h>
#include <utils/portlist.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QDir>
#include <QEventLoop>
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

const char VBOXMANAGE[] = "VBoxManage";
const char LIST[] = "list";
const char HDDS[] = "hdds";
const char RUNNINGVMS[] = "runningvms";
const char VMS[] = "vms";
const char SHOWVMINFO[] = "showvminfo";
const char MACHINE_READABLE[] = "--machinereadable";
const char STARTVM[] = "startvm";
const char CONTROLVM[] = "controlvm";
const char ACPI_POWER_BUTTON[] = "acpipowerbutton";
const char TYPE[] = "--type";
const char HEADLESS[] = "headless";
const char SHAREDFOLDER[] = "sharedfolder";
const char SHARE_NAME[] = "--name";
const char REMOVE_SHARED[] = "remove";
const char HOSTPATH[] = "--hostpath";
const char ADD_SHARED[] = "add";
const char GETEXTRADATA[] = "getextradata";
const char SETEXTRADATA[] = "setextradata";
const char CUSTOM_VIDEO_MODE1[] = "CustomVideoMode1";
const char LAST_GUEST_SIZE_HINT[] = "GUI/LastGuestSizeHint";
const char AUTORESIZE_GUEST[] = "GUI/AutoresizeGuest";
const char ENABLE_SYMLINKS[] = "VBoxInternal2/SharedFoldersEnableSymlinksCreate/%1";
const char YES_ARG[] = "1";
const char MODIFYVM[] = "modifyvm";
const char NATPF1[] = "--natpf1";
const char DELETE[] = "delete";
const char QML_LIVE_NATPF_RULE_NAME_MATCH[] = "qmllive_";
const char FREE_PORT_NATPF_RULE_NAME_MATCH[] = "freeport_";
const char NATPF_RULE_TEMPLATE[] = "%1,tcp,127.0.0.1,%2,,%3";
const char QML_LIVE_NATPF_RULE_NAME_TEMPLATE[] = "qmllive_%1";
const char SSH_NATPF_RULE_NAME[] = "guestssh";
const char WWW_NATPF_RULE_NAME[] = "guestwww";
const char MODIFYMEDIUM[] = "modifymedium";
const char RESIZE[] = "--resize";
const char MEMORY[] = "--memory";
const char CPUS[] = "--cpus";
const char SHOWMEDIUMINFO[] = "showmediuminfo";
const char METRICS[] = "metrics";
const char COLLECT[] = "collect";
const char TOTAL_RAM[] = "RAM/Usage/Total";
const char SNAPSHOT[] = "snapshot";
const char RESTORE[] = "restore";

namespace Sfdk {

class VBoxVirtualMachineInfo : public VirtualMachineInfo
{
public:
    QString vdiUuid;

    // VdiInfo
    QStringList allRelatedVdiUuids;
};

static VBoxVirtualMachineInfo virtualMachineInfoFromOutput(const QString &output);
static void vdiInfoFromOutput(const QString &output, VBoxVirtualMachineInfo *virtualMachineInfo);
static void snapshotInfoFromOutput(const QString &output, VBoxVirtualMachineInfo *virtualMachineInfo);
static int ramSizeFromOutput(const QString &output, bool *matched);
static bool isVirtualMachineRunningFromInfo(const QString &vmInfo, bool *headless);
static QStringList listedVirtualMachines(const QString &output);

static QString vBoxManagePath()
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

    QTC_ASSERT(!path.isEmpty(), return path);
    return path;
}

class VBoxManageRunner : public ProcessRunner
{
    Q_OBJECT

public:
    explicit VBoxManageRunner(const QStringList &arguments, QObject *parent = 0)
        : ProcessRunner(vBoxManagePath(), arguments, parent)
    {
    }
};

CommandQueue *commandQueue()
{
    return SdkPrivate::commandQueue();
}

void VirtualBoxManager::probe(const QString &vmName, const QObject *context,
        const Functor<VirtualMachinePrivate::BasicState, bool> &functor)
{
    Q_ASSERT(context);
    Q_ASSERT(functor);

    QStringList arguments;
    arguments.append(QLatin1String(SHOWVMINFO));
    arguments.append(vmName);

    auto runner = std::make_unique<VBoxManageRunner>(arguments);
    connect(runner->process(), QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), context,
            [=, runner = runner.get()](int exitCode, QProcess::ExitStatus exitStatus) {
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

    commandQueue()->enqueue(std::move(runner));
}

// It is an error to call this function when the VM vmName is running
void VirtualBoxManager::updateSharedFolder(const QString &vmName,
        VirtualMachinePrivate::SharedPath which, const QString &newFolder,
        const QObject *context, const Functor<bool> &functor)
{
    Q_ASSERT(context);
    Q_ASSERT(functor);

    qCDebug(vms) << "Setting shared folder" << which << "for" << vmName << "to" << newFolder;

    QString mountName;
    switch (which) {
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
        break;
    }

    const QPointer<const QObject> context_{context};

    QStringList rargs;
    rargs.append(QLatin1String(SHAREDFOLDER));
    rargs.append(QLatin1String(REMOVE_SHARED));
    rargs.append(vmName);
    rargs.append(QLatin1String(SHARE_NAME));
    rargs.append(mountName);

    QStringList aargs;
    aargs.append(QLatin1String(SHAREDFOLDER));
    aargs.append(QLatin1String(ADD_SHARED));
    aargs.append(vmName);
    aargs.append(QLatin1String(SHARE_NAME));
    aargs.append(mountName);
    aargs.append(QLatin1String(HOSTPATH));
    aargs.append(newFolder);

    QStringList sargs;
    sargs.append(QLatin1String(SETEXTRADATA));
    sargs.append(vmName);
    sargs.append(QString::fromLatin1(ENABLE_SYMLINKS).arg(mountName));
    sargs.append(QLatin1String(YES_ARG));

    auto enqueue = [=](const QStringList &args, CommandQueue::BatchId batch) {
        auto runner = std::make_unique<VBoxManageRunner>(args);
        connect(runner.get(), &VBoxManageRunner::failure, Sdk::instance(), [=]() {
            commandQueue()->cancelBatch(batch);
            callIf(context_, functor, false);
        });
        VBoxManageRunner *runner_ = runner.get();
        commandQueue()->enqueue(std::move(runner));
        return runner_;
    };

    CommandQueue::BatchId batch = commandQueue()->beginBatch();
    enqueue(rargs, batch);
    enqueue(aargs, batch);
    VBoxManageRunner *last = enqueue(sargs, batch);
    commandQueue()->endBatch();

    connect(last, &VBoxManageRunner::success, context, std::bind(functor, true));
}

// It is an error to call this function when the VM vmName is running
void VirtualBoxManager::updateReservedPortForwarding(const QString &vmName,
        VirtualMachinePrivate::ReservedPort which, quint16 port,
        const QObject *context, const Functor<bool> &functor)
{
    Q_ASSERT(context);
    Q_ASSERT(functor);

    qCDebug(vms) << "Setting reserved port forwarding" << which << "for" << vmName << "to" << port;

    QString ruleName;
    int guestPort;
    switch (which) {
        case VirtualMachinePrivate::SshPort:
            ruleName = QLatin1String(SSH_NATPF_RULE_NAME);
            guestPort = 22;
            break;
        case VirtualMachinePrivate::WwwPort:
            ruleName = QLatin1String(WWW_NATPF_RULE_NAME);
            guestPort = 8080;
            break;
    }
    const QString rule = QString::fromLatin1(NATPF_RULE_TEMPLATE)
        .arg(ruleName).arg(port).arg(guestPort);

    QStringList arguments;
    arguments.append(QLatin1String(MODIFYVM));
    arguments.append(vmName);
    arguments.append(QLatin1String(NATPF1));
    arguments.append(QLatin1String(DELETE));
    arguments.append(ruleName);
    arguments.append(QLatin1String(NATPF1));
    arguments.append(rule);

    auto runner = std::make_unique<VBoxManageRunner>(arguments);
    connect(runner.get(), &VBoxManageRunner::done, context, functor);
    commandQueue()->enqueue(std::move(runner));
}

void VirtualBoxManager::fetchVirtualMachineInfo(const QString &vmName,
        VirtualMachineInfo::ExtraInfos extraInfo,
        const QObject *context, const Functor<const VirtualMachineInfo &, bool> &functor)
{
    Q_ASSERT(context);
    Q_ASSERT(functor);

    const QPointer<const QObject> context_{context};

    auto enqueue = [=](const QStringList &arguments, CommandQueue::BatchId batch, auto onSuccess) {
        auto runner = std::make_unique<VBoxManageRunner>(arguments);
        connect(runner.get(), &VBoxManageRunner::failure, Sdk::instance(), [=]() {
            commandQueue()->cancelBatch(batch);
            callIf(context_, functor, VirtualMachineInfo{}, false);
        });
        connect(runner.get(), &VBoxManageRunner::success,
                context, std::bind(onSuccess, runner.get()));
        VBoxManageRunner *runner_ = runner.get();
        commandQueue()->enqueue(std::move(runner));
        return runner_;
    };

    CommandQueue::BatchId batch = commandQueue()->beginBatch();

    auto info = std::make_shared<VBoxVirtualMachineInfo>();
    VBoxManageRunner *last;

    QStringList arguments;
    arguments.append(QLatin1String(SHOWVMINFO));
    arguments.append(vmName);
    arguments.append(QLatin1String(MACHINE_READABLE));

    last = enqueue(arguments, batch, [=](VBoxManageRunner *runner) {
        // FIXME return by argument
        *info = virtualMachineInfoFromOutput(
                QString::fromLocal8Bit(runner->process()->readAllStandardOutput()));
    });

    if (extraInfo & VirtualMachineInfo::VdiInfo) {
        QStringList arguments;
        arguments.append(QLatin1String(LIST));
        arguments.append(QLatin1String(HDDS));

        last = enqueue(arguments, batch, [=](VBoxManageRunner *runner) {
            vdiInfoFromOutput(QString::fromLocal8Bit(runner->process()->readAllStandardOutput()), info.get());
        });
    }

    if (extraInfo & VirtualMachineInfo::SnapshotInfo) {
        QStringList arguments;
        arguments.append(QLatin1String(SNAPSHOT));
        arguments.append(vmName);
        arguments.append(QLatin1String(LIST));
        arguments.append(QLatin1String(MACHINE_READABLE));

        last = enqueue(arguments, batch, [=](VBoxManageRunner *runner) {
            snapshotInfoFromOutput(QString::fromLocal8Bit(runner->process()->readAllStandardOutput()),
                    info.get());
        });
    }

    commandQueue()->endBatch();

    connect(last, &VBoxManageRunner::success, context, [=]() { functor(*info, true); });
}

// It is an error to call this function when the VM vmName is running
void VirtualBoxManager::startVirtualMachine(const QString &vmName, bool headless,
        const QObject *context, const Functor<bool> &functor)
{
    Q_ASSERT(context);
    Q_ASSERT(functor);

    QStringList arguments;
    arguments.append(QLatin1String(STARTVM));
    arguments.append(vmName);
    if (headless) {
        arguments.append(QLatin1String(TYPE));
        arguments.append(QLatin1String(HEADLESS));
    }

    auto runner = std::make_unique<VBoxManageRunner>(arguments);
    connect(runner.get(), &VBoxManageRunner::done, context, functor);
    commandQueue()->enqueue(std::move(runner));
}

// It is an error to call this function when the VM vmName is not running
void VirtualBoxManager::shutVirtualMachine(const QString &vmName, const QObject *context,
            const Functor<bool> &functor)
{
    Q_ASSERT(context);
    Q_ASSERT(functor);

    QStringList arguments;
    arguments.append(QLatin1String(CONTROLVM));
    arguments.append(vmName);
    arguments.append(QLatin1String(ACPI_POWER_BUTTON));

    auto runner = std::make_unique<VBoxManageRunner>(arguments);
    connect(runner.get(), &VBoxManageRunner::done, context, functor);
    commandQueue()->enqueue(std::move(runner));
}

// accepts void slot(bool ok)
void VirtualBoxManager::restoreSnapshot(const QString &vmName, const QString &snapshotName,
            const QObject *context, const Functor<bool> &functor)
{
    Q_ASSERT(context);
    Q_ASSERT(functor);

    QStringList arguments;
    arguments.append(QLatin1String(SNAPSHOT));
    arguments.append(vmName);
    arguments.append(QLatin1String(RESTORE));
    arguments.append(snapshotName);

    auto runner = std::make_unique<VBoxManageRunner>(arguments);
    connect(runner.get(), &VBoxManageRunner::done, context, functor);
    commandQueue()->enqueue(std::move(runner));
}

void VirtualBoxManager::fetchRegisteredVirtualMachines(const QObject *context,
        const Functor<const QStringList &, bool> &functor)
{
    Q_ASSERT(context);
    Q_ASSERT(functor);

    QStringList arguments;
    arguments.append(QLatin1String(LIST));
    arguments.append(QLatin1String(VMS));

    auto runner = std::make_unique<VBoxManageRunner>(arguments);
    connect(runner.get(), &VBoxManageRunner::done,
            context, [=, runner = runner.get()](bool ok) {
        if (!ok) {
            functor({}, false);
            return;
        }
        QStringList vms = listedVirtualMachines(QString::fromLocal8Bit(
                runner->process()->readAllStandardOutput()));
        functor(vms, true);
    });
    commandQueue()->enqueue(std::move(runner));
}

void VirtualBoxManager::setVideoMode(const QString &vmName, const QSize &size, int depth,
        const QObject *context, const Functor<bool> &functor)
{
    Q_ASSERT(context);
    Q_ASSERT(functor);

    const QPointer<const QObject> context_{context};

    QString videoMode = QStringLiteral("%1x%2x%3")
        .arg(size.width())
        .arg(size.height())
        .arg(depth);
    QString hint = QStringLiteral("%1,%2")
        .arg(size.width())
        .arg(size.height());

    auto allOk = std::make_shared<bool>(true);
    auto enqueue = [=](const QString &key, const QString &value, bool isLast = false) {
        setExtraData(vmName, key, value, Sdk::instance(), [=](bool ok) {
            if (!ok)
                *allOk = false;
            if (isLast)
                callIf(context_, functor, *allOk);
        });
    };

    enqueue(QLatin1String(CUSTOM_VIDEO_MODE1), videoMode);
    enqueue(QLatin1String(LAST_GUEST_SIZE_HINT), hint);
    enqueue(QLatin1String(AUTORESIZE_GUEST), QLatin1Literal("false"), true /* mind me! */);
}

void VirtualBoxManager::setVdiCapacityMb(const QString &vmName, int sizeMb, const QObject *context,
        const Functor<bool> &functor)
{
    Q_ASSERT(context);
    Q_ASSERT(functor);

    qCDebug(vms) << "Changing vdi size of" << vmName << "to" << sizeMb << "MB";

    const QPointer<const QObject> context_{context};
    fetchVirtualMachineInfo(vmName, VirtualMachineInfo::VdiInfo, Sdk::instance(),
            [=](const VirtualMachineInfo &virtualMachineInfo_, bool ok) {
        if (!ok) {
            callIf(context_, functor, false);
            return;
        }
        auto virtualMachineInfo = static_cast<const VBoxVirtualMachineInfo &>(virtualMachineInfo_);

        if (sizeMb < virtualMachineInfo.vdiCapacityMb) {
            qWarning() << "VBoxManage failed to" << MODIFYMEDIUM << virtualMachineInfo.vdiUuid << RESIZE << sizeMb
                       << "for VM" << vmName << ". Can't reduce VDI. Current size:" << virtualMachineInfo.vdiCapacityMb;
            callIf(context_, functor, false);
            return;
        } else if (sizeMb == virtualMachineInfo.vdiCapacityMb) {
            callIf(context_, functor, true);
            return;
        }

        if (virtualMachineInfo.allRelatedVdiUuids.isEmpty()) {
            // Something went wrong, an error message should have been already issued
            callIf(context_, functor, false);
            return;
        }

        QStringList toResize = virtualMachineInfo.allRelatedVdiUuids;
        qCDebug(vms) << "About to resize these VDIs (in order):" << toResize;

        auto allOk = std::make_shared<bool>(true);
        while (!toResize.isEmpty()) {
            // take last - enqueueImmediate reverses the actual order of execution
            const QString vdiUuid = toResize.takeLast();
            const bool isLast = toResize.isEmpty();

            QStringList arguments;
            arguments.append(QLatin1String(MODIFYMEDIUM));
            arguments.append(vdiUuid);
            arguments.append(QLatin1String(RESIZE));
            arguments.append(QString::number(sizeMb));

            auto runner = std::make_unique<VBoxManageRunner>(arguments);
            connect(runner.get(), &VBoxManageRunner::done, Sdk::instance(), [=](bool ok) {
                if (!ok) {
                    qWarning() << "VBoxManage failed to" << MODIFYMEDIUM << vdiUuid << RESIZE;
                    *allOk = false;
                }
                if (isLast)
                    callIf(context_, functor, *allOk);
            });
            commandQueue()->enqueueImmediate(std::move(runner));
        }
    });
}

// It is an error to call this function when the VM vmName is running
void VirtualBoxManager::setMemorySizeMb(const QString &vmName, int sizeMb, const QObject *context,
        const Functor<bool> &functor)
{
    Q_ASSERT(context);
    Q_ASSERT(functor);

    qCDebug(vms) << "Changing memory size of" << vmName << "to" << sizeMb << "MB";

    QStringList arguments;
    arguments.append(QLatin1String(MODIFYVM));
    arguments.append(vmName);
    arguments.append(QLatin1String(MEMORY));
    arguments.append(QString::number(sizeMb));

    auto runner = std::make_unique<VBoxManageRunner>(arguments);
    connect(runner.get(), &VBoxManageRunner::done, context, functor);
    commandQueue()->enqueue(std::move(runner));
}

void VirtualBoxManager::setCpuCount(const QString &vmName, int count, const QObject *context,
        const Functor<bool> &functor)
{
    Q_ASSERT(context);
    Q_ASSERT(functor);

    qCDebug(vms) << "Changing CPU count of" << vmName << "to" << count;

    QStringList arguments;
    arguments.append(QLatin1String(MODIFYVM));
    arguments.append(vmName);
    arguments.append(QLatin1String(CPUS));
    arguments.append(QString::number(count));

    auto runner = std::make_unique<VBoxManageRunner>(arguments);
    connect(runner.get(), &VBoxManageRunner::done, context, functor);
    commandQueue()->enqueue(std::move(runner));
}

void VirtualBoxManager::fetchExtraData(const QString &vmName, const QString &key,
        const QObject *context, const Functor<QString, bool> &functor)
{
    Q_ASSERT(context);
    Q_ASSERT(functor);

    QStringList arguments;
    arguments.append(QLatin1String(GETEXTRADATA));
    arguments.append(vmName);
    arguments.append(key);

    auto runner = std::make_unique<VBoxManageRunner>(arguments);
    connect(runner.get(), &VBoxManageRunner::done,
            context, [=, runner = runner.get()](bool ok) {
        if (!ok) {
            functor({}, false);
            return;
        }
        auto data = QString::fromLocal8Bit(runner->process()->readAllStandardOutput());
        functor(data, true);
    });
    commandQueue()->enqueue(std::move(runner));
}

void VirtualBoxManager::fetchHostTotalMemorySizeMb(const QObject *context,
            const Functor<int, bool> &functor)
{
    Q_ASSERT(context);
    Q_ASSERT(functor);

    QStringList arguments;
    arguments.clear();
    arguments.append(QLatin1String(METRICS));
    arguments.append(QLatin1String(COLLECT));
    arguments.append(QLatin1String("host"));
    arguments.append(QLatin1String(TOTAL_RAM));

    auto runner = std::make_unique<VBoxManageRunner>(arguments);
    QMetaObject::Connection failureConnection = connect(runner.get(), &VBoxManageRunner::failure,
            context, std::bind(functor, 0, false));
    connect(runner->process(), &QProcess::readyReadStandardOutput,
            context, [=, runner = runner.get()](){
        bool matched;
        auto memSizeKb = ramSizeFromOutput(QString::fromLocal8Bit(runner->process()->readAllStandardOutput()),
                &matched);
        if (!matched)
            return;
        QObject::disconnect(failureConnection);
        runner->process()->closeReadChannel(QProcess::StandardOutput);
        runner->terminate();
        functor(memSizeKb / 1024, true);
    });

    connect(context, &QObject::destroyed, runner.get(), &VBoxManageRunner::terminate);

    commandQueue()->enqueue(std::move(runner));
}

void VirtualBoxManager::setExtraData(const QString &vmName, const QString &keyword,
        const QString &data, const QObject *context, const Functor<bool> &functor)
{
    Q_ASSERT(context);
    Q_ASSERT(functor);

    QStringList args;
    args.append(QLatin1String(SETEXTRADATA));
    args.append(vmName);
    args.append(keyword);
    args.append(data);

    auto runner = std::make_unique<VBoxManageRunner>(args);
    connect(runner.get(), &VBoxManageRunner::done, context, functor);
    commandQueue()->enqueue(std::move(runner));
}

// It is an error to call this function when the VM vmName is running
void VirtualBoxManager::deletePortForwardingRule(const QString &vmName, const QString &ruleName,
        const QObject *context, const Functor<bool> &functor)
{
    Q_ASSERT(context);
    Q_ASSERT(functor);

    qCDebug(vms) << "Deleting port forwarding rule" << ruleName << "from" << vmName;

    QStringList arguments;
    arguments.append(QLatin1String(MODIFYVM));
    arguments.append(vmName);
    arguments.append(QLatin1String(NATPF1));
    arguments.append(QLatin1String(DELETE));
    arguments.append(ruleName);

    auto runner = std::make_unique<VBoxManageRunner>(arguments);
    connect(runner.get(), &VBoxManageRunner::done, context, functor);
    commandQueue()->enqueue(std::move(runner));
}

// It is an error to call this function when the VM vmName is running
void VirtualBoxManager::updatePortForwardingRule(const QString &vmName, const QString &protocol,
        const QString &ruleName, quint16 hostPort, quint16 vmPort,
        const QObject *context, const Functor<bool> &functor)
{
    Q_ASSERT(context);
    Q_ASSERT(functor);

    deletePortForwardingRule(vmName, ruleName, Sdk::instance(), [](bool) {/* noop */});

    qCDebug(vms) << "Setting port forwarding for" << vmName << "from"
        << hostPort << "to" << vmPort;

    QStringList arguments;
    arguments.append(QLatin1String(MODIFYVM));
    arguments.append(vmName);
    arguments.append(QLatin1String(NATPF1));
    arguments.append(QString::fromLatin1("%1,%2,,%3,,%4")
                     .arg(ruleName).arg(protocol).arg(hostPort).arg(vmPort));

    auto runner = std::make_unique<VBoxManageRunner>(arguments);
    connect(runner.get(), &VBoxManageRunner::done, context, functor);
    commandQueue()->enqueueImmediate(std::move(runner));
}

// It is an error to call this function when the VM vmName is running
void VirtualBoxManager::updateReservedPortListForwarding(const QString &vmName,
        VirtualMachinePrivate::ReservedPortList which, const QList<Utils::Port> &ports,
        const QObject *context, const Functor<const QMap<QString, quint16> &, bool> &functor)
{
    Q_ASSERT(context);
    Q_ASSERT(functor);

    qCDebug(vms) << "Setting reserved port forwarding" << which << "for" << vmName << "to" << ports;

    QString ruleNameTemplate;
    switch (which) {
    case VirtualMachinePrivate::QmlLivePortList:
        ruleNameTemplate = QLatin1String(QML_LIVE_NATPF_RULE_NAME_TEMPLATE);
        break;
    }
    const QString ruleTemplate = QString::fromLatin1(NATPF_RULE_TEMPLATE).arg(ruleNameTemplate);

    for (int i = 1; i <= Constants::MAX_QML_LIVE_PORTS; ++i) {
        QStringList arguments;
        arguments.append(QLatin1String(MODIFYVM));
        arguments.append(vmName);
        arguments.append(QLatin1String(NATPF1));
        arguments.append(QLatin1String(DELETE));
        arguments.append(ruleNameTemplate.arg(i));

        auto runner = std::make_unique<VBoxManageRunner>(arguments);
        connect(runner.get(), &VBoxManageRunner::failure, [=]() {
            qWarning() << "VBoxManage failed to delete" << which << "port #" << QString::number(i);
        });
        commandQueue()->enqueue(std::move(runner));
    }

    auto savedPorts = std::make_shared<QMap<QString, quint16>>();

    QList<quint16> ports_ = Utils::transform(ports, &Port::number);
    std::sort(ports_.begin(), ports_.end());

    int i = 1;
    VBoxManageRunner *lastSetRunner = nullptr;
    foreach (quint16 port, ports_) {
        QStringList arguments;
        arguments.append(QLatin1String(MODIFYVM));
        arguments.append(vmName);
        arguments.append(QLatin1String(NATPF1));
        arguments.append(ruleTemplate.arg(i).arg(port).arg(port));

        auto runner = std::make_unique<VBoxManageRunner>(arguments);
        connect(runner.get(), &VBoxManageRunner::done, Sdk::instance(), [=](bool ok) {
            if (ok)
                savedPorts->insert(ruleNameTemplate.arg(i), port);
            else
                qWarning() << "VBoxManage failed to set" << which << "port" << port;
        });
        commandQueue()->enqueue(std::move(runner));
        ++i;
        lastSetRunner = runner.get();
    }

    connect(lastSetRunner, &VBoxManageRunner::done, context, [=](bool ok) {
        Q_UNUSED(ok);
        functor(*savedPorts, savedPorts->values() == ports_);
    });
}

bool isVirtualMachineRunningFromInfo(const QString &vmInfo, bool *headless)
{
    Q_ASSERT(headless);
    // VBox 4.x says "type", 5.x says "name"
    QRegularExpression re(QStringLiteral("^Session (name|type):\\s*(\\w+)"), QRegularExpression::MultilineOption);
    QRegularExpressionMatch match = re.match(vmInfo);
    *headless = match.captured(2) == QLatin1String("headless");
    return match.hasMatch();
}

QStringList listedVirtualMachines(const QString &output)
{
    QStringList vms;
    QRegExp rx(QLatin1String("\"(.*)\""));
    rx.setMinimal(true);
    int pos = 0;
    while ((pos = rx.indexIn(output, pos)) != -1) {
        pos += rx.matchedLength();
        vms.append(rx.cap(1));
    }
    return vms;
}

VBoxVirtualMachineInfo virtualMachineInfoFromOutput(const QString &output)
{
    VBoxVirtualMachineInfo info;
    info.sshPort = 0;

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
            else if (ruleName.contains(QLatin1String(WWW_NATPF_RULE_NAME)))
                info.wwwPort = port;
            else if (ruleName.contains(QLatin1String(QML_LIVE_NATPF_RULE_NAME_MATCH)))
                info.qmlLivePorts[ruleName] = port;
            else if (ruleName.contains(QLatin1String(FREE_PORT_NATPF_RULE_NAME_MATCH)))
                info.freePorts[ruleName] = port;
            else
                info.otherPorts[ruleName] = port;
        } else if(rexp.cap(0).startsWith(QLatin1String("SharedFolderNameMachineMapping"))) {
            if (rexp.cap(7) == QLatin1String("home"))
                info.sharedHome = rexp.cap(8);
            else if (rexp.cap(7) == QLatin1String("targets"))
                info.sharedTargets = rexp.cap(8);
            else if (rexp.cap(7) == QLatin1String("ssh"))
                info.sharedSsh = rexp.cap(8);
            else if (rexp.cap(7) == QLatin1String("config"))
                info.sharedConfig = rexp.cap(8);
            else if (rexp.cap(7).startsWith(QLatin1String("src")))
                info.sharedSrc = rexp.cap(8);
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
            QString vdiUuid = rexp.cap(12);
            if (!vdiUuid.isEmpty() && vdiUuid != QLatin1String("none")) {
                info.vdiUuid = vdiUuid;
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

void vdiInfoFromOutput(const QString &output, VBoxVirtualMachineInfo *virtualMachineInfo)
{
    // 1 UUID
    // 2 Parent UUID
    // 3 VDI capacity
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
            if (currentUuid != virtualMachineInfo->vdiUuid)
                continue;
            QString capacityStr = rexp.cap(3);
            QTC_ASSERT(!capacityStr.isEmpty(), continue);
            bool ok;
            int capacity = capacityStr.toInt(&ok);
            QTC_CHECK(ok);
            virtualMachineInfo->vdiCapacityMb = ok ? capacity : 0;
        }
    }

    QTC_ASSERT(parentAndChildrenUuids.contains(virtualMachineInfo->vdiUuid), return);

    QSet<QString> relatedUuids;
    QStack<QString> uuidsToCheck;
    uuidsToCheck.push(virtualMachineInfo->vdiUuid);
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

    virtualMachineInfo->allRelatedVdiUuids = relatedUuids.toList();
}

int ramSizeFromOutput(const QString &output, bool *matched)
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

void snapshotInfoFromOutput(const QString &output, VBoxVirtualMachineInfo *virtualMachineInfo)
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

} // Sfdk

#include "virtualboxmanager.moc"
