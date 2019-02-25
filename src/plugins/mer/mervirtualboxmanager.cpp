/****************************************************************************
**
** Copyright (C) 2012 - 2016 Jolla Ltd.
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

#include "mervirtualboxmanager.h"

#include "merconstants.h"
#include "merlogging.h"

#include <utils/hostosinfo.h>
#include <utils/portlist.h>
#include <utils/qtcassert.h>

#include <QBasicTimer>
#include <QDebug>
#include <QDir>
#include <QProcess>
#include <QQueue>
#include <QRegularExpression>
#include <QSettings>
#include <QSize>
#include <QTime>
#include <QTimer>
#include <QTimerEvent>
#include <QThread>

#include <algorithm>

using namespace Utils;

const char VBOXMANAGE[] = "VBoxManage";
const char LIST[] = "list";
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
const char ENABLE_SYMLINKS[] = "VBoxInternal2/SharedFoldersEnableSymlinksCreate/%1";
const char YES_ARG[] = "1";
const char MODIFYVM[] = "modifyvm";
const char NATPF1[] = "--natpf1";
const char DELETE[] = "delete";
const char QML_LIVE_NATPF_RULE_NAME_MATCH[] = "qmllive_";
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

namespace Mer {
namespace Internal {

static VirtualMachineInfo virtualMachineInfoFromOutput(const QString &output, bool fillVdiInfo);
static void fetchVdiCapacity(VirtualMachineInfo *virtualMachineInfo);
static void vdiCapacityFromOutput(const QString &output, VirtualMachineInfo *virtualMachineInfo);
static int ramSizeFromOutput(const QString &output);
static bool isVirtualMachineRunningFromInfo(const QString &vmInfo);
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
        QTC_ASSERT(!s_instance, return);
        s_instance = this;
    }

    ~CommandSerializer() override
    {
        s_instance = 0;
    }

    static bool runSynchronous(QProcess *process)
    {
        s_instance->m_queue.prepend(process);

        // Currently runnning an asynchronous process?
        if (s_instance->m_current)
            s_instance->m_current->waitForFinished();

        s_instance->dequeue();
        QTC_CHECK(s_instance->m_current == process);

        return s_instance->m_current->waitForFinished();
    }

    static void runAsynchronous(QProcess *process)
    {
        s_instance->m_queue.enqueue(process);
        s_instance->scheduleDequeue();
    }

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
        if (m_queue.isEmpty())
            return;

        m_current = m_queue.dequeue();

        connect(m_current, &QProcess::errorOccurred, this, &CommandSerializer::finalize);
        void (QProcess::*QProcess_finished)(int, QProcess::ExitStatus) = &QProcess::finished;
        connect(m_current, QProcess_finished, this, &CommandSerializer::finalize);

        m_current->start(QIODevice::ReadWrite | QIODevice::Text);
    }

private slots:
    void finalize()
    {
        QTC_ASSERT(sender() == m_current, return);

        m_current->disconnect(this);
        m_current = 0;

        scheduleDequeue();
    }

private:
    static CommandSerializer *s_instance;
    QQueue<QProcess *> m_queue;
    QProcess *m_current{};
    QBasicTimer m_dequeueTimer;
};

CommandSerializer *CommandSerializer::s_instance = 0;

class VBoxManageProcess : public QProcess
{
    Q_OBJECT

public:
    explicit VBoxManageProcess(QObject *parent = 0)
        : QProcess(parent)
    {
        setProcessChannelMode(QProcess::ForwardedErrorChannel);
        setProgram(vBoxManagePath());
    }

    bool runSynchronously(const QStringList &arguments)
    {
        setArguments(arguments);
        bool finished = CommandSerializer::runSynchronous(this);
        return finished && exitStatus() == QProcess::NormalExit && exitCode() == 0;
    }

    void runAsynchronously(const QStringList &arguments)
    {
        setArguments(arguments);
        CommandSerializer::runAsynchronous(this);
    }

    void setDeleteOnFinished()
    {
        void (QProcess::*QProcess_finished)(int, QProcess::ExitStatus) = &QProcess::finished;
        connect(this, &QProcess::errorOccurred, this, &QObject::deleteLater);
        connect(this, QProcess_finished, this, &QObject::deleteLater);
    }
};

MerVirtualBoxManager *MerVirtualBoxManager::m_instance = 0;

MerVirtualBoxManager::MerVirtualBoxManager(QObject *parent):
    QObject(parent)
{
    m_instance = this;
    new CommandSerializer(this);
}

MerVirtualBoxManager::~MerVirtualBoxManager()
{
    m_instance = 0;
}

MerVirtualBoxManager *MerVirtualBoxManager::instance()
{
    QTC_CHECK(m_instance);
    return m_instance;
}

// accepts void slot(bool running, bool exists)
void MerVirtualBoxManager::isVirtualMachineRunning(const QString &vmName, QObject *context,
                                                   std::function<void(bool, bool)> slot)
{
    QStringList arguments;
    arguments.append(QLatin1String(SHOWVMINFO));
    arguments.append(vmName);

    VBoxManageProcess *process = new VBoxManageProcess(instance());

    void (QProcess::*QProcess_finished)(int, QProcess::ExitStatus) = &QProcess::finished;
    connect(process, QProcess_finished, context,
            [process, vmName, slot](int exitCode, QProcess::ExitStatus exitStatus) {
                Q_UNUSED(exitCode);
                Q_UNUSED(exitStatus);
                bool isRunning = isVirtualMachineRunningFromInfo(
                    QString::fromLocal8Bit(process->readAllStandardOutput()));
                bool exists = exitCode == 0;
                slot(isRunning, exists);
            });

    process->setDeleteOnFinished();
    process->runAsynchronously(arguments);
}

// It is an error to call this function when the VM vmName is running
bool MerVirtualBoxManager::updateSharedFolder(const QString &vmName, const QString &mountName, const QString &newFolder)
{
    QStringList rargs;
    rargs.append(QLatin1String(SHAREDFOLDER));
    rargs.append(QLatin1String(REMOVE_SHARED));
    rargs.append(vmName);
    rargs.append(QLatin1String(SHARE_NAME));
    rargs.append(mountName);

    VBoxManageProcess rproc;
    if (!rproc.runSynchronously(rargs)) {
        qWarning() << "VBoxManage failed to remove " << mountName;
        return false;
    }

    QStringList aargs;
    aargs.append(QLatin1String(SHAREDFOLDER));
    aargs.append(QLatin1String(ADD_SHARED));
    aargs.append(vmName);
    aargs.append(QLatin1String(SHARE_NAME));
    aargs.append(mountName);
    aargs.append(QLatin1String(HOSTPATH));
    aargs.append(newFolder);

    VBoxManageProcess aproc;
    if (!aproc.runSynchronously(aargs)) {
        qWarning() << "VBoxManage failed to add " << mountName;
        return false;
    }

    QStringList sargs;
    sargs.append(QLatin1String(SETEXTRADATA));
    sargs.append(vmName);
    sargs.append(QString::fromLatin1(ENABLE_SYMLINKS).arg(mountName));
    sargs.append(QLatin1String(YES_ARG));

    VBoxManageProcess sproc;
    if (!sproc.runSynchronously(sargs)) {
        qWarning() << "VBoxManage failed to enable symlinks under " << mountName;
        return false;
    }

    return true;
}

// It is an error to call this function when the VM vmName is running
bool MerVirtualBoxManager::updateSdkSshPort(const QString &vmName, quint16 port)
{
    qCDebug(Log::vms) << "Setting SSH port forwarding for" << vmName << "to" << port;

    QStringList arguments;
    arguments.append(QLatin1String(MODIFYVM));
    arguments.append(vmName);
    arguments.append(QLatin1String(NATPF1));
    arguments.append(QLatin1String(DELETE));
    arguments.append(QLatin1String(SDK_SSH_NATPF_RULE_NAME));
    arguments.append(QLatin1String(NATPF1));
    arguments.append(QString::fromLatin1(SDK_SSH_NATPF_RULE_TEMPLATE).arg(port));

    QTime timer;
    timer.start();

    VBoxManageProcess process;
    if (!process.runSynchronously(arguments)) {
        qWarning() << "VBoxManage failed to" << MODIFYVM;
        return false;
    }

    qCDebug(Log::vms) << "Setting SSH port forwarding took" << timer.elapsed() << "milliseconds";

    return true;
}

// It is an error to call this function when the VM vmName is running
bool MerVirtualBoxManager::updateSdkWwwPort(const QString &vmName, quint16 port)
{
    qCDebug(Log::vms) << "Setting WWW port forwarding for" << vmName << "to" << port;

    QStringList arguments;
    arguments.append(QLatin1String(MODIFYVM));
    arguments.append(vmName);
    arguments.append(QLatin1String(NATPF1));
    arguments.append(QLatin1String(DELETE));
    arguments.append(QLatin1String(SDK_WWW_NATPF_RULE_NAME));
    arguments.append(QLatin1String(NATPF1));
    arguments.append(QString::fromLatin1(SDK_WWW_NATPF_RULE_TEMPLATE).arg(port));

    QTime timer;
    timer.start();

    VBoxManageProcess process;
    if (!process.runSynchronously(arguments)) {
        qWarning() << "VBoxManage failed to" << MODIFYVM;
        return false;
    }

    qCDebug(Log::vms) << "Setting WWW port forwarding took" << timer.elapsed() << "milliseconds";

    return true;
}

// It is an error to call this function when the VM vmName is running
bool MerVirtualBoxManager::updateEmulatorSshPort(const QString &vmName, quint16 port)
{
    qCDebug(Log::vms) << "Setting SSH port forwarding for" << vmName << "to" << port;

    QStringList arguments;
    arguments.append(QLatin1String(MODIFYVM));
    arguments.append(vmName);
    arguments.append(QLatin1String(NATPF1));
    arguments.append(QLatin1String(DELETE));
    arguments.append(QLatin1String(EMULATOR_SSH_NATPF_RULE_NAME));
    arguments.append(QLatin1String(NATPF1));
    arguments.append(QString::fromLatin1(EMULATOR_SSH_NATPF_RULE_TEMPLATE).arg(port));

    QTime timer;
    timer.start();

    VBoxManageProcess process;
    if (!process.runSynchronously(arguments)) {
        qWarning() << "VBoxManage failed to" << MODIFYVM;
        return false;
    }

    qCDebug(Log::vms) << "Setting SSH port forwarding took" << timer.elapsed() << "milliseconds";

    return true;
}

VirtualMachineInfo MerVirtualBoxManager::fetchVirtualMachineInfo(const QString &vmName, bool fetchVdiInfo)
{
    VirtualMachineInfo info;
    QStringList arguments;
    arguments.append(QLatin1String(SHOWVMINFO));
    arguments.append(vmName);
    arguments.append(QLatin1String(MACHINE_READABLE));
    VBoxManageProcess process;
    if (!process.runSynchronously(arguments))
        return info;

    return virtualMachineInfoFromOutput(QString::fromLocal8Bit(process.readAllStandardOutput()), fetchVdiInfo);
}

// It is an error to call this function when the VM vmName is running
void MerVirtualBoxManager::startVirtualMachine(const QString &vmName,bool headless)
{
    QStringList arguments;
    arguments.append(QLatin1String(STARTVM));
    arguments.append(vmName);
    if (headless) {
        arguments.append(QLatin1String(TYPE));
        arguments.append(QLatin1String(HEADLESS));
    }

    VBoxManageProcess *process = new VBoxManageProcess(instance());
    process->setDeleteOnFinished();
    process->runAsynchronously(arguments);
}

// It is an error to call this function when the VM vmName is not running
void MerVirtualBoxManager::shutVirtualMachine(const QString &vmName)
{
    QStringList arguments;
    arguments.append(QLatin1String(CONTROLVM));
    arguments.append(vmName);
    arguments.append(QLatin1String(ACPI_POWER_BUTTON));

    VBoxManageProcess *process = new VBoxManageProcess(instance());
    process->setDeleteOnFinished();
    process->runAsynchronously(arguments);
}

QStringList MerVirtualBoxManager::fetchRegisteredVirtualMachines()
{
    QStringList vms;
    QStringList arguments;
    arguments.append(QLatin1String(LIST));
    arguments.append(QLatin1String(VMS));
    VBoxManageProcess process;
    if (!process.runSynchronously(arguments))
        return vms;

    return listedVirtualMachines(QString::fromLocal8Bit(process.readAllStandardOutput()));
}

// It is an error to call this function when the VM vmName is running
void MerVirtualBoxManager::setVideoMode(const QString &vmName, const QSize &size, int depth)
{
    QString videoMode = QStringLiteral("%1x%2x%3")
        .arg(size.width())
        .arg(size.height())
        .arg(depth);

    QStringList args;
    args.append(QLatin1String(SETEXTRADATA));
    args.append(vmName);
    args.append(QLatin1String(CUSTOM_VIDEO_MODE1));
    args.append(videoMode);

    VBoxManageProcess *process = new VBoxManageProcess(instance());
    process->setDeleteOnFinished();
    process->runAsynchronously(args);
}

void MerVirtualBoxManager::setVdiCapacityMb(const QString &vmName, int sizeMb, QObject *context, std::function<void(bool)> slot)
{
    qCDebug(Log::vms) << "Changing vdi size of" << vmName << "to" << sizeMb << "MB";

    const VirtualMachineInfo virtualMachineInfo = fetchVirtualMachineInfo(vmName, true);
    if (sizeMb < virtualMachineInfo.vdiCapacityMb) {
        qWarning() << "VBoxManage failed to" << MODIFYMEDIUM << virtualMachineInfo.vdiPath << RESIZE << sizeMb
                   << "for VM" << vmName << ". Can't reduce VDI. Current size:" << virtualMachineInfo.vdiCapacityMb;
        QTimer::singleShot(0, context, [slot]() { slot(false); });
        return;
    } else if (sizeMb == virtualMachineInfo.vdiCapacityMb) {
        QTimer::singleShot(0, context, [slot]() { slot(true); });
        return;
    }

    QStringList arguments;
    arguments.append(QLatin1String(MODIFYMEDIUM));
    arguments.append(virtualMachineInfo.vdiPath);
    arguments.append(QLatin1String(RESIZE));
    arguments.append(QString::number(sizeMb));

    QTime timer;
    timer.start();
    VBoxManageProcess *process = new VBoxManageProcess(instance());

    connect(process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), context,
                [&timer, slot](int exitCode, QProcess::ExitStatus exitStatus) {
                    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
                        qCDebug(Log::vms) << "Resizing VDI took" << timer.elapsed() << "milliseconds";
                    } else {
                        qWarning() << "VBoxManage failed to" << MODIFYMEDIUM << RESIZE;
                    }
                    slot(exitStatus == QProcess::NormalExit && exitCode == 0);
                });

    process->setDeleteOnFinished();
    process->runAsynchronously(arguments);
}

// It is an error to call this function when the VM vmName is running
bool MerVirtualBoxManager::setMemorySizeMb(const QString &vmName, int sizeMb)
{
    qCDebug(Log::vms) << "Changing memory size of" << vmName << "to" << sizeMb << "MB";

    QStringList arguments;
    arguments.append(QLatin1String(MODIFYVM));
    arguments.append(vmName);
    arguments.append(QLatin1String(MEMORY));
    arguments.append(QString::number(sizeMb));

    QTime timer;
    timer.start();

    VBoxManageProcess process;
    if (!process.runSynchronously(arguments)) {
        qWarning() << "VBoxManage failed to" << MODIFYVM << vmName << MEMORY << sizeMb;
        return false;
    }

    qCDebug(Log::vms) << "Resizing memory took" << timer.elapsed() << "milliseconds";

    return true;
}

bool MerVirtualBoxManager::setCpuCount(const QString &vmName, int count)
{
    qCDebug(Log::vms) << "Changing CPU count of" << vmName << "to" << count;

    QStringList arguments;
    arguments.append(QLatin1String(MODIFYVM));
    arguments.append(vmName);
    arguments.append(QLatin1String(CPUS));
    arguments.append(QString::number(count));

    QTime timer;
    timer.start();

    VBoxManageProcess process;
    if (!process.runSynchronously(arguments)) {
        qWarning() << "VBoxManage failed to" << MODIFYVM << vmName << CPUS << count;
        return false;
    }

    qCDebug(Log::vms) << "Changing CPU count took" << timer.elapsed() << "milliseconds";

    return true;
}

QString MerVirtualBoxManager::getExtraData(const QString &vmName, const QString &key)
{
    QStringList arguments;
    arguments.append(QLatin1String(GETEXTRADATA));
    arguments.append(vmName);
    arguments.append(key);
    VBoxManageProcess process;
    if (!process.runSynchronously(arguments)) {
        qWarning() << "VBoxManage failed to getextradata";
        return QString();
    }

    return QString::fromLocal8Bit(process.readAllStandardOutput());
}

void MerVirtualBoxManager::getHostTotalMemorySizeMb(QObject *context, std::function<void(int)> slot)
{
    QStringList arguments;
    arguments.clear();
    arguments.append(QLatin1String(METRICS));
    arguments.append(QLatin1String(COLLECT));
    arguments.append(QLatin1String("host"));
    arguments.append(QLatin1String(TOTAL_RAM));

    auto process = new VBoxManageProcess(instance());
    connect(process, &QProcess::readyReadStandardOutput, context, [process, slot](){
        auto memSizeKb = ramSizeFromOutput(QString::fromLocal8Bit(process->readAllStandardOutput()));
        process->terminate();
        slot(memSizeKb / 1024);
    });

    connect(context, &QObject::destroyed, process, &QProcess::terminate);

    process->setDeleteOnFinished();
    process->runAsynchronously(arguments);
}

int MerVirtualBoxManager::getHostTotalCpuCount()
{
    return QThread::idealThreadCount();
}

// It is an error to call this function when the VM vmName is running
Utils::PortList MerVirtualBoxManager::updateEmulatorQmlLivePorts(const QString &vmName, const QList<Utils::Port> &ports)
{
    qCDebug(Log::vms) << "Setting QmlLive port forwarding for" << vmName << "to" << ports;

    QTime timer;
    timer.start();

    for (int i = 1; i <= Constants::MAX_QML_LIVE_PORTS; ++i) {
        QStringList arguments;
        arguments.append(QLatin1String(MODIFYVM));
        arguments.append(vmName);
        arguments.append(QLatin1String(NATPF1));
        arguments.append(QLatin1String(DELETE));
        arguments.append(QString::fromLatin1(QML_LIVE_NATPF_RULE_NAME_TEMPLATE).arg(i));

        VBoxManageProcess process;
        if (!process.runSynchronously(arguments))
            qWarning() << "VBoxManage failed to delete QmlLive port #" << QString::number(i);
    }

    Utils::PortList savedPorts;

    auto ports_ = ports;
    std::sort(ports_.begin(), ports_.end());

    int i = 1;
    foreach (const Utils::Port &port, ports_) {
        QStringList arguments;
        arguments.append(QLatin1String(MODIFYVM));
        arguments.append(vmName);
        arguments.append(QLatin1String(NATPF1));
        arguments.append(QString::fromLatin1(QML_LIVE_NATPF_RULE_TEMPLATE).arg(i).arg(port.number()));

        VBoxManageProcess process;
        if (!process.runSynchronously(arguments)) {
            qWarning() << "VBoxManage failed to set QmlLive port" << port.toString();
            continue;
        }

        savedPorts.addPort(port);
        ++i;
    }

    qCDebug(Log::vms) << "Setting QmlLive port forwarding took" << timer.elapsed() << "milliseconds";

    return savedPorts;
}

bool isVirtualMachineRunningFromInfo(const QString &vmInfo)
{
    // VBox 4.x says "type", 5.x says "name"
    QRegularExpression re(QStringLiteral("^Session (name|type):"), QRegularExpression::MultilineOption);
    return re.match(vmInfo).hasMatch();
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

VirtualMachineInfo virtualMachineInfoFromOutput(const QString &output, bool fillVdiInfo)
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
                               "|(?:\"SATA-\\d-\\d\"=\"(.*)\")"
                               "|(?:memory=(\\d+)\\n)"
                               "|(?:cpus=(\\d+)\\n)"
                               ));

    rexp.setMinimal(true);
    int pos = 0;
    while ((pos = rexp.indexIn(output, pos)) != -1) {
        pos += rexp.matchedLength();
        if (rexp.cap(0).startsWith(QLatin1String("Forwarding"))) {
            quint16 port = rexp.cap(4).toUInt();
            if (rexp.cap(1).contains(QLatin1String(SDK_SSH_NATPF_RULE_NAME)))
                info.sshPort = port;
            else if (rexp.cap(1).contains(QLatin1String(SDK_WWW_NATPF_RULE_NAME)))
                info.wwwPort = port;
            else if (rexp.cap(1).contains(QLatin1String(QML_LIVE_NATPF_RULE_NAME_MATCH)))
                info.qmlLivePorts << port;
            else
                info.freePorts << port;
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
        } else if (rexp.cap(0).startsWith(QLatin1String("\"SATA")) && fillVdiInfo) {
            QString vdiPath = rexp.cap(12);
            if (!vdiPath.isEmpty() && vdiPath != QLatin1String("none")) {
                info.vdiPath = vdiPath;
                fetchVdiCapacity(&info);
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

void fetchVdiCapacity(VirtualMachineInfo *virtualMachineInfo)
{
    QStringList arguments;
    arguments.append(QLatin1String(SHOWMEDIUMINFO));
    arguments.append(virtualMachineInfo->vdiPath);
    VBoxManageProcess process;
    if (!process.runSynchronously(arguments))
        return;

    vdiCapacityFromOutput(QString::fromLocal8Bit(process.readAllStandardOutput()), virtualMachineInfo);
}

void vdiCapacityFromOutput(const QString &output, VirtualMachineInfo *virtualMachineInfo)
{
    // 1 vdi capacity
    QRegExp rexp(QLatin1String("(?:Capacity:\\s+(\\d+) MBytes)"));

    rexp.setMinimal(true);
    int pos = 0;
    while ((pos = rexp.indexIn(output, pos)) != -1) {
        pos += rexp.matchedLength();
        if (rexp.cap(0).startsWith(QLatin1String("Capacity"))) {
            QString capacityStr = rexp.cap(1);
            if (!capacityStr.isEmpty()) {
                bool ok;
                int capacity = capacityStr.toInt(&ok);
                virtualMachineInfo->vdiCapacityMb = ok ? capacity : 0;
            }
        }
    }
}

int ramSizeFromOutput(const QString &output)
{
    QRegExp rexp(QLatin1String("(RAM/Usage/Total\\s+(\\d+) kB)"));

    rexp.setMinimal(true);
    int pos = 0;
    while ((pos = rexp.indexIn(output, pos)) != -1) {
        pos += rexp.matchedLength();
        int memSize = rexp.cap(2).toInt();
        if (memSize > 0) {
            return memSize;
        }
    }
    return 0;
}

} // Internal
} // VirtualBox

#include "mervirtualboxmanager.moc"
