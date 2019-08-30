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

#include "sfdkconstants.h"

#include <utils/hostosinfo.h>
#include <utils/portlist.h>
#include <utils/qtcassert.h>

#include <QBasicTimer>
#include <QDebug>
#include <QDir>
#include <QEventLoop>
#include <QProcess>
#include <QQueue>
#include <QRegularExpression>
#include <QSettings>
#include <QStack>
#include <QSize>
#include <QTime>
#include <QTimer>
#include <QTimerEvent>
#include <QThread>
#include <QUuid>

#include <deque>

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
const char QML_LIVE_NATPF_RULE_NAME_TEMPLATE[] = "qmllive_%1";
const char QML_LIVE_NATPF_RULE_TEMPLATE[] = "qmllive_%1,tcp,127.0.0.1,%2,,%2";
const char SDK_SSH_NATPF_RULE_NAME[] = "guestssh";
const char SDK_SSH_NATPF_RULE_TEMPLATE[] = "guestssh,tcp,127.0.0.1,%1,,22";
const char SDK_WWW_NATPF_RULE_NAME[] = "guestwww";
const char SDK_WWW_NATPF_RULE_TEMPLATE[] = "guestwww,tcp,127.0.0.1,%1,,9292";
const char EMULATOR_SSH_NATPF_RULE_NAME[] = "guestssh";
const char EMULATOR_SSH_NATPF_RULE_TEMPLATE[] = "guestssh,tcp,127.0.0.1,%1,,22";
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

const int TERMINATE_TIMEOUT_MS = 3000;

namespace Sfdk {

static VirtualMachineInfo virtualMachineInfoFromOutput(const QString &output);
static void vdiInfoFromOutput(const QString &output, VirtualMachineInfo *virtualMachineInfo);
static void snapshotInfoFromOutput(const QString &output, VirtualMachineInfo *virtualMachineInfo);
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

class CommandSerializer : public QObject
{
    Q_OBJECT

public:
    explicit CommandSerializer(QObject *parent)
        : QObject(parent)
    {
    }

    ~CommandSerializer() override
    {
        QTC_CHECK(m_queue.empty());
    }

    void wait()
    {
        if (!m_queue.empty()) {
            QEventLoop loop;
            connect(this, &CommandSerializer::empty, &loop, &QEventLoop::quit);
            loop.exec();
        }
    }

    using BatchId = int;
    BatchId beginBatch()
    {
        m_lastBatchId++;
        if (m_lastBatchId < 0)
            m_lastBatchId = 1;
        m_currentBatchId = m_lastBatchId;
        return m_currentBatchId;
    }

    void endBatch()
    {
        m_currentBatchId = -1;
    }

    void cancelBatch(BatchId batchId)
    {
        while (!m_queue.empty() && m_queue.front().first == batchId) {
            QProcess *process = m_queue.front().second.get();
            qCDebug(vmsQueue) << "Canceled" << (void*)process
                << process->program() << process->arguments();
            m_queue.pop_front();
        }
    }

    void enqueue(std::unique_ptr<QProcess> &&process)
    {
        QTC_ASSERT(process, return);
        qCDebug(vmsQueue) << "Enqueued" << (void*)process.get()
            << process->program() << process->arguments();
        m_queue.emplace_back(m_currentBatchId, std::move(process));
        scheduleDequeue();
    }

    void enqueueImmediate(std::unique_ptr<QProcess> &&process)
    {
        QTC_ASSERT(process, return);
        qCDebug(vmsQueue) << "Enqueued (immediate)" << (void*)process.get()
            << process->program() << process->arguments();
        m_queue.emplace_front(m_currentBatchId, std::move(process));
        scheduleDequeue();
    }

signals:
    void empty();

protected:
    void timerEvent(QTimerEvent *event) override
    {
        if (event->timerId() == m_dequeueTimer.timerId()) {
            m_dequeueTimer.stop();
            dequeue();
        } else  {
            QObject::timerEvent(event);
        }
    }

private:
    void scheduleDequeue()
    {
        m_dequeueTimer.start(0, this);
    }

    void dequeue()
    {
        if (m_current)
            return;
        if (m_queue.empty()) {
            emit empty();
            return;
        }

        m_current = std::move(m_queue.front().second);
        m_queue.pop_front();

        qCDebug(vmsQueue) << "Dequeued" << (void*)m_current.get();

        connect(m_current.get(), &QProcess::errorOccurred,
                this, &CommandSerializer::finalize);
        connect(m_current.get(), QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, &CommandSerializer::finalize);

        m_current->start(QIODevice::ReadWrite | QIODevice::Text);
    }

private slots:
    void finalize()
    {
        qCDebug(vmsQueue) << "Finished" << (void*)sender()
            << "current:" << (void*)m_current.get();
        QTC_ASSERT(sender() == m_current.get(), return);

        m_current->disconnect(this);
        m_current->deleteLater();
        m_current.release();

        scheduleDequeue();
    }

private:
    std::deque<std::pair<BatchId, std::unique_ptr<QProcess>>> m_queue;
    BatchId m_lastBatchId = 0;
    BatchId m_currentBatchId = -1;
    std::unique_ptr<QProcess> m_current;
    QBasicTimer m_dequeueTimer;
};

class VBoxManageProcess : public QProcess
{
    Q_OBJECT

public:
    explicit VBoxManageProcess(const QStringList &arguments, QObject *parent = 0)
        : QProcess(parent)
    {
        setProcessChannelMode(QProcess::ForwardedErrorChannel);
        setProgram(vBoxManagePath());
        setArguments(arguments);
        connect(this, &QProcess::errorOccurred,
                this, &VBoxManageProcess::onErrorOccured);
        connect(this, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, &VBoxManageProcess::onFinished);
    }

    QList<int> expectedExitCodes() const { return m_expectedExitCodes; }
    void setExpectedExitCodes(const QList<int> &expectedExitCodes)
    {
        m_expectedExitCodes = expectedExitCodes;
    }

public slots:
    void reallyTerminate()
    {
        QTC_CHECK(state() != Starting);
        if (state() == NotRunning)
            return;

        m_crashExpected = true;

        terminate();
        if (!m_terminateTimeoutTimer.isActive())
            m_terminateTimeoutTimer.start(TERMINATE_TIMEOUT_MS, this);
    }

signals:
    void success();
    void failure();
    void done(bool ok);

protected:
    void timerEvent(QTimerEvent *event) override
    {
        if (event->timerId() == m_terminateTimeoutTimer.timerId()) {
            m_terminateTimeoutTimer.stop();
            // Note that on Windows it always ends here as terminate() has no
            // effect on VBoxManage there
            kill();
        } else {
            QProcess::timerEvent(event);
        }
    }

private slots:
    void onErrorOccured(QProcess::ProcessError error)
    {
        if (error == QProcess::FailedToStart) {
            qCWarning(vms) << "VBoxManage failed to start";
            emit failure();
            emit done(false);
        }
    }

    void onFinished(int exitCode, QProcess::ExitStatus exitStatus)
    {
        m_terminateTimeoutTimer.stop();

        if (exitStatus != QProcess::NormalExit) {
            if (m_crashExpected) {
                qCDebug(vms) << "VBoxManage crashed as expected. Arguments:" << arguments();
                emit success();
                emit done(true);
                return;
            }
            qCWarning(vms) << "VBoxManage crashed. Arguments:" << arguments();
            emit failure();
            emit done(false);
        } else if (!m_expectedExitCodes.contains(exitCode)) {
            qCWarning(vms) << "VBoxManage exited with unexpected exit code" << exitCode
                << ". Arguments:" << arguments();
            emit failure();
            emit done(false);
        } else {
            emit success();
            emit done(true);
        }
    }

private:
    QList<int> m_expectedExitCodes = {0};
    bool m_crashExpected = false;
    QBasicTimer m_terminateTimeoutTimer;
};

VirtualBoxManager *VirtualBoxManager::s_instance = 0;

VirtualBoxManager::VirtualBoxManager(QObject *parent)
    : QObject(parent)
    , m_serializer(std::make_unique<CommandSerializer>(this))
{
    s_instance = this;
}

VirtualBoxManager::~VirtualBoxManager()
{
    m_serializer->wait();
    s_instance = 0;
}

VirtualBoxManager *VirtualBoxManager::instance()
{
    QTC_CHECK(s_instance);
    return s_instance;
}

void VirtualBoxManager::probe(const QString &vmName, const QObject *context,
        const Functor<VirtualMachinePrivate::BasicState, bool> &functor)
{
    Q_ASSERT(context);
    Q_ASSERT(functor);

    QStringList arguments;
    arguments.append(QLatin1String(SHOWVMINFO));
    arguments.append(vmName);

    auto process = std::make_unique<VBoxManageProcess>(arguments);
    connect(process.get(), QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), context,
            [=, process = process.get()](int exitCode, QProcess::ExitStatus exitStatus) {
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
                    QString::fromLocal8Bit(process->readAllStandardOutput()), &isHeadless);
                if (isRunning)
                    state |= VirtualMachinePrivate::Running;
                if (isHeadless)
                    state |= VirtualMachinePrivate::Headless;
                functor(state, true);
            });

    s_instance->m_serializer->enqueue(std::move(process));
}

// It is an error to call this function when the VM vmName is running
void VirtualBoxManager::updateSharedFolder(const QString &vmName, const QString &mountName,
            const QString &newFolder, const QObject *context, const Functor<bool> &functor)
{
    Q_ASSERT(context);
    Q_ASSERT(functor);

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

    auto enqueue = [=](const QStringList &args, CommandSerializer::BatchId batch) {
        auto process = std::make_unique<VBoxManageProcess>(args);
        connect(process.get(), &VBoxManageProcess::failure, s_instance, [=]() {
            s_instance->m_serializer->cancelBatch(batch);
            callIf(context_, functor, false);
        });
        VBoxManageProcess *process_ = process.get();
        s_instance->m_serializer->enqueue(std::move(process));
        return process_;
    };

    CommandSerializer::BatchId batch = s_instance->m_serializer->beginBatch();
    enqueue(rargs, batch);
    enqueue(aargs, batch);
    VBoxManageProcess *last = enqueue(sargs, batch);
    s_instance->m_serializer->endBatch();

    connect(last, &VBoxManageProcess::success, context, std::bind(functor, true));
}

// It is an error to call this function when the VM vmName is running
void VirtualBoxManager::updateSdkSshPort(const QString &vmName, quint16 port,
        const QObject *context, const Functor<bool> &functor)
{
    Q_ASSERT(context);
    Q_ASSERT(functor);

    qCDebug(vms) << "Setting SSH port forwarding for" << vmName << "to" << port;

    QStringList arguments;
    arguments.append(QLatin1String(MODIFYVM));
    arguments.append(vmName);
    arguments.append(QLatin1String(NATPF1));
    arguments.append(QLatin1String(DELETE));
    arguments.append(QLatin1String(SDK_SSH_NATPF_RULE_NAME));
    arguments.append(QLatin1String(NATPF1));
    arguments.append(QString::fromLatin1(SDK_SSH_NATPF_RULE_TEMPLATE).arg(port));

    auto process = std::make_unique<VBoxManageProcess>(arguments);
    connect(process.get(), &VBoxManageProcess::done, context, functor);
    s_instance->m_serializer->enqueue(std::move(process));
}

// It is an error to call this function when the VM vmName is running
void VirtualBoxManager::updateSdkWwwPort(const QString &vmName, quint16 port,
        const QObject *context, const Functor<bool> &functor)
{
    Q_ASSERT(context);
    Q_ASSERT(functor);

    qCDebug(vms) << "Setting WWW port forwarding for" << vmName << "to" << port;

    QStringList arguments;
    arguments.append(QLatin1String(MODIFYVM));
    arguments.append(vmName);
    arguments.append(QLatin1String(NATPF1));
    arguments.append(QLatin1String(DELETE));
    arguments.append(QLatin1String(SDK_WWW_NATPF_RULE_NAME));
    arguments.append(QLatin1String(NATPF1));
    arguments.append(QString::fromLatin1(SDK_WWW_NATPF_RULE_TEMPLATE).arg(port));

    auto process = std::make_unique<VBoxManageProcess>(arguments);
    connect(process.get(), &VBoxManageProcess::done, context, functor);
    s_instance->m_serializer->enqueue(std::move(process));
}

// It is an error to call this function when the VM vmName is running
void VirtualBoxManager::updateEmulatorSshPort(const QString &vmName, quint16 port,
        const QObject *context, const Functor<bool> &functor)
{
    Q_ASSERT(context);
    Q_ASSERT(functor);

    qCDebug(vms) << "Setting SSH port forwarding for" << vmName << "to" << port;

    QStringList arguments;
    arguments.append(QLatin1String(MODIFYVM));
    arguments.append(vmName);
    arguments.append(QLatin1String(NATPF1));
    arguments.append(QLatin1String(DELETE));
    arguments.append(QLatin1String(EMULATOR_SSH_NATPF_RULE_NAME));
    arguments.append(QLatin1String(NATPF1));
    arguments.append(QString::fromLatin1(EMULATOR_SSH_NATPF_RULE_TEMPLATE).arg(port));

    auto process = std::make_unique<VBoxManageProcess>(arguments);
    connect(process.get(), &VBoxManageProcess::done, context, functor);
    s_instance->m_serializer->enqueue(std::move(process));
}

void VirtualBoxManager::fetchVirtualMachineInfo(const QString &vmName, ExtraInfos extraInfo,
        const QObject *context, const Functor<const VirtualMachineInfo &, bool> &functor)
{
    Q_ASSERT(context);
    Q_ASSERT(functor);

    const QPointer<const QObject> context_{context};

    auto enqueue = [=](const QStringList &arguments, CommandSerializer::BatchId batch, auto onSuccess) {
        auto process = std::make_unique<VBoxManageProcess>(arguments);
        connect(process.get(), &VBoxManageProcess::failure, s_instance, [=]() {
            s_instance->m_serializer->cancelBatch(batch);
            callIf(context_, functor, VirtualMachineInfo{}, false);
        });
        connect(process.get(), &VBoxManageProcess::success,
                context, std::bind(onSuccess, process.get()));
        VBoxManageProcess *process_ = process.get();
        s_instance->m_serializer->enqueue(std::move(process));
        return process_;
    };

    CommandSerializer::BatchId batch = s_instance->m_serializer->beginBatch();

    auto info = std::make_shared<VirtualMachineInfo>();
    VBoxManageProcess *last;

    QStringList arguments;
    arguments.append(QLatin1String(SHOWVMINFO));
    arguments.append(vmName);
    arguments.append(QLatin1String(MACHINE_READABLE));

    last = enqueue(arguments, batch, [=](VBoxManageProcess *process) {
        // FIXME return by argument
        *info = virtualMachineInfoFromOutput(
                QString::fromLocal8Bit(process->readAllStandardOutput()));
    });

    if (extraInfo & VdiInfo) {
        QStringList arguments;
        arguments.append(QLatin1String(LIST));
        arguments.append(QLatin1String(HDDS));

        last = enqueue(arguments, batch, [=](VBoxManageProcess *process) {
            vdiInfoFromOutput(QString::fromLocal8Bit(process->readAllStandardOutput()), info.get());
        });
    }

    if (extraInfo & SnapshotInfo) {
        QStringList arguments;
        arguments.append(QLatin1String(SNAPSHOT));
        arguments.append(vmName);
        arguments.append(QLatin1String(LIST));
        arguments.append(QLatin1String(MACHINE_READABLE));

        last = enqueue(arguments, batch, [=](VBoxManageProcess *process) {
            snapshotInfoFromOutput(QString::fromLocal8Bit(process->readAllStandardOutput()),
                    info.get());
        });
    }

    s_instance->m_serializer->endBatch();

    connect(last, &VBoxManageProcess::success, context, [=]() { functor(*info, true); });
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

    auto process = std::make_unique<VBoxManageProcess>(arguments);
    connect(process.get(), &VBoxManageProcess::done, context, functor);
    s_instance->m_serializer->enqueue(std::move(process));
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

    auto process = std::make_unique<VBoxManageProcess>(arguments);
    connect(process.get(), &VBoxManageProcess::done, context, functor);
    s_instance->m_serializer->enqueue(std::move(process));
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

    auto process = std::make_unique<VBoxManageProcess>(arguments);
    connect(process.get(), &VBoxManageProcess::done, context, functor);
    s_instance->m_serializer->enqueue(std::move(process));
}

void VirtualBoxManager::fetchRegisteredVirtualMachines(const QObject *context,
        const Functor<const QStringList &, bool> &functor)
{
    Q_ASSERT(context);
    Q_ASSERT(functor);

    QStringList arguments;
    arguments.append(QLatin1String(LIST));
    arguments.append(QLatin1String(VMS));

    auto process = std::make_unique<VBoxManageProcess>(arguments);
    connect(process.get(), &VBoxManageProcess::done,
            context, [=, process = process.get()](bool ok) {
        if (!ok) {
            functor({}, false);
            return;
        }
        QStringList vms = listedVirtualMachines(QString::fromLocal8Bit(
                process->readAllStandardOutput()));
        functor(vms, true);
    });
    s_instance->m_serializer->enqueue(std::move(process));
}

// It is an error to call this function when the VM vmName is running
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
        setExtraData(vmName, key, value, s_instance, [=](bool ok) {
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
    fetchVirtualMachineInfo(vmName, VdiInfo, s_instance,
            [=](const VirtualMachineInfo &virtualMachineInfo, bool ok) {
        if (!ok) {
            callIf(context_, functor, false);
            return;
        }
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

            auto process = std::make_unique<VBoxManageProcess>(arguments);
            connect(process.get(), &VBoxManageProcess::done, s_instance, [=](bool ok) {
                if (!ok) {
                    qWarning() << "VBoxManage failed to" << MODIFYMEDIUM << vdiUuid << RESIZE;
                    *allOk = false;
                }
                if (isLast)
                    callIf(context_, functor, *allOk);
            });
            s_instance->m_serializer->enqueueImmediate(std::move(process));
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

    auto process = std::make_unique<VBoxManageProcess>(arguments);
    connect(process.get(), &VBoxManageProcess::done, context, functor);
    s_instance->m_serializer->enqueue(std::move(process));
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

    auto process = std::make_unique<VBoxManageProcess>(arguments);
    connect(process.get(), &VBoxManageProcess::done, context, functor);
    s_instance->m_serializer->enqueue(std::move(process));
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

    auto process = std::make_unique<VBoxManageProcess>(arguments);
    connect(process.get(), &VBoxManageProcess::done,
            context, [=, process = process.get()](bool ok) {
        if (!ok) {
            functor({}, false);
            return;
        }
        auto data = QString::fromLocal8Bit(process->readAllStandardOutput());
        functor(data, true);
    });
    s_instance->m_serializer->enqueue(std::move(process));
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

    auto process = std::make_unique<VBoxManageProcess>(arguments);
    QMetaObject::Connection failureConnection = connect(process.get(), &VBoxManageProcess::failure,
            context, std::bind(functor, 0, false));
    connect(process.get(), &QProcess::readyReadStandardOutput,
            context, [=, process = process.get()](){
        bool matched;
        auto memSizeKb = ramSizeFromOutput(QString::fromLocal8Bit(process->readAllStandardOutput()),
                &matched);
        if (!matched)
            return;
        QObject::disconnect(failureConnection);
        process->closeReadChannel(QProcess::StandardOutput);
        process->reallyTerminate();
        functor(memSizeKb / 1024, true);
    });

    connect(context, &QObject::destroyed, process.get(), &VBoxManageProcess::reallyTerminate);

    s_instance->m_serializer->enqueue(std::move(process));
}

int VirtualBoxManager::getHostTotalCpuCount()
{
    return QThread::idealThreadCount();
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

    auto process = std::make_unique<VBoxManageProcess>(args);
    connect(process.get(), &VBoxManageProcess::done, context, functor);
    s_instance->m_serializer->enqueue(std::move(process));
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

    auto process = std::make_unique<VBoxManageProcess>(arguments);
    connect(process.get(), &VBoxManageProcess::done, context, functor);
    s_instance->m_serializer->enqueue(std::move(process));
}

// It is an error to call this function when the VM vmName is running
void VirtualBoxManager::updatePortForwardingRule(const QString &vmName, const QString &protocol,
        const QString &ruleName, quint16 hostPort, quint16 vmPort,
        const QObject *context, const Functor<bool> &functor)
{
    Q_ASSERT(context);
    Q_ASSERT(functor);

    deletePortForwardingRule(vmName, ruleName, s_instance, [](bool) {/* noop */});

    qCDebug(vms) << "Setting port forwarding for" << vmName << "from"
        << hostPort << "to" << vmPort;

    QStringList arguments;
    arguments.append(QLatin1String(MODIFYVM));
    arguments.append(vmName);
    arguments.append(QLatin1String(NATPF1));
    arguments.append(QString::fromLatin1("%1,%2,,%3,,%4")
                     .arg(ruleName).arg(protocol).arg(hostPort).arg(vmPort));

    auto process = std::make_unique<VBoxManageProcess>(arguments);
    connect(process.get(), &VBoxManageProcess::done, context, functor);
    s_instance->m_serializer->enqueueImmediate(std::move(process));
}

void VirtualBoxManager::fetchPortForwardingRules(const QString &vmName, const QObject *context,
        const Functor<const QList<QMap<QString, quint16>> &, bool> &functor)
{
    Q_ASSERT(context);
    Q_ASSERT(functor);

    fetchVirtualMachineInfo(vmName, VdiInfo,
            context, [=](const VirtualMachineInfo &vmInfo, bool ok) {
        if (!ok) {
            functor({}, false);
            return;
        }
        auto rules = QList<QMap<QString, quint16>>({vmInfo.otherPorts, vmInfo.qmlLivePorts,
                vmInfo.freePorts});
        functor(rules, true);
    });
}

// It is an error to call this function when the VM vmName is running
void VirtualBoxManager::updateEmulatorQmlLivePorts(const QString &vmName,
        const QList<Utils::Port> &ports, const QObject *context,
        const Functor<const Utils::PortList &, bool> &functor)
{
    Q_ASSERT(context);
    Q_ASSERT(functor);

    qCDebug(vms) << "Setting QmlLive port forwarding for" << vmName << "to" << ports;

    for (int i = 1; i <= Constants::MAX_QML_LIVE_PORTS; ++i) {
        QStringList arguments;
        arguments.append(QLatin1String(MODIFYVM));
        arguments.append(vmName);
        arguments.append(QLatin1String(NATPF1));
        arguments.append(QLatin1String(DELETE));
        arguments.append(QString::fromLatin1(QML_LIVE_NATPF_RULE_NAME_TEMPLATE).arg(i));

        auto process = std::make_unique<VBoxManageProcess>(arguments);
        connect(process.get(), &VBoxManageProcess::failure, [=]() {
            qWarning() << "VBoxManage failed to delete QmlLive port #" << QString::number(i);
        });
        s_instance->m_serializer->enqueue(std::move(process));
    }

    auto savedPorts = std::make_shared<Utils::PortList>();

    auto ports_ = ports;
    std::sort(ports_.begin(), ports_.end());

    int i = 1;
    VBoxManageProcess *lastSetProcess = nullptr;
    foreach (const Utils::Port &port, ports_) {
        QStringList arguments;
        arguments.append(QLatin1String(MODIFYVM));
        arguments.append(vmName);
        arguments.append(QLatin1String(NATPF1));
        arguments.append(QString::fromLatin1(QML_LIVE_NATPF_RULE_TEMPLATE).arg(i).arg(port.number()));

        auto process = std::make_unique<VBoxManageProcess>(arguments);
        connect(process.get(), &VBoxManageProcess::done, s_instance, [=](bool ok) {
            if (ok)
                savedPorts->addPort(port);
            else
                qWarning() << "VBoxManage failed to set QmlLive port" << port.toString();
        });
        s_instance->m_serializer->enqueue(std::move(process));
        ++i;
        lastSetProcess = process.get();
    }

    connect(lastSetProcess, &VBoxManageProcess::done, context, [=](bool ok) {
        Q_UNUSED(ok);

        auto isPortListEqual = [] (Utils::PortList l1, const QList<Utils::Port> &l2) {
            if (l1.count() != l2.count())
                return false;

            while (l1.hasMore()) {
                const Port current = l1.getNext();
                if (!l2.contains(current))
                    return false;
            }
            return true;
        };

        functor(*savedPorts, isPortListEqual(*savedPorts, ports_));
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

VirtualMachineInfo virtualMachineInfoFromOutput(const QString &output)
{
    VirtualMachineInfo info;
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
            if (ruleName.contains(QLatin1String(SDK_SSH_NATPF_RULE_NAME)))
                info.sshPort = port;
            else if (ruleName.contains(QLatin1String(SDK_WWW_NATPF_RULE_NAME)))
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

void vdiInfoFromOutput(const QString &output, VirtualMachineInfo *virtualMachineInfo)
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

void snapshotInfoFromOutput(const QString &output, VirtualMachineInfo *virtualMachineInfo)
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
