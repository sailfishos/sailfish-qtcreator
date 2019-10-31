/****************************************************************************
**
** Copyright (C) 2016-2019 Jolla Ltd.
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

#include "merqmllivebenchmanager.h"

#include <set>

#include <QCoreApplication>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QTimer>

#include <sfdk/sfdkconstants.h>

#include <coreplugin/icore.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>
#include <ssh/sshconnection.h>
#include <utils/portlist.h>
#include <utils/qtcassert.h>

#include "merconstants.h"
#include "merdevice.h"
#include "meremulatordevice.h"
#include "merlogging.h"
#include "merrunconfigurationaspect.h"
#include "mersettings.h"

using namespace Core;
using namespace ProjectExplorer;
using namespace Sfdk;

namespace Mer {
namespace Internal {

namespace {
const char VALUE_SEPARATOR = ',';
const char ADD_HOST_OPTION[] = "--addhost";
const char PING_OPTION[] = "--ping";
const char PROBE_HOST_OPTION[] = "--probehost";
const char REMOTE_ONLY_OPTION[] = "--remoteonly";
const char RM_HOST_OPTION[] = "--rmhost";

const char APP_READY_PATTERN[] = "qmlliveruntime-sailfish initialized";
const int PROBE_PERIOD_MS = 2000;
const int PROBE_TIMEOUT_MS = 20000;
}

MerQmlLiveBenchManager *MerQmlLiveBenchManager::m_instance = nullptr;

MerQmlLiveBenchManager::MerQmlLiveBenchManager(QObject *parent)
    : QObject(parent)
    , m_enabled{false}
{
    m_instance = this;

    onBenchLocationChanged();
    connect(MerSettings::instance(), &MerSettings::qmlLiveBenchLocationChanged,
            this, &MerQmlLiveBenchManager::onBenchLocationChanged);
    connect(MerSettings::instance(), &MerSettings::syncQmlLiveWorkspaceEnabledChanged,
        this, [this](bool enabled) {
            if (enabled)
                onStartupProjectChanged(SessionManager::startupProject());
        });

    onDeviceListReplaced();
    connect(DeviceManager::instance(), &DeviceManager::deviceAdded,
            this, &MerQmlLiveBenchManager::onDeviceAdded);
    connect(DeviceManager::instance(), &DeviceManager::deviceRemoved,
            this, &MerQmlLiveBenchManager::onDeviceRemoved);
    connect(DeviceManager::instance(), &DeviceManager::deviceListReplaced,
            this, &MerQmlLiveBenchManager::onDeviceListReplaced);

    onStartupProjectChanged(SessionManager::startupProject());
    connect(SessionManager::instance(), &SessionManager::startupProjectChanged,
            this, &MerQmlLiveBenchManager::onStartupProjectChanged);

    connect(ProjectExplorerPlugin::instance(), &ProjectExplorerPlugin::aboutToExecuteProject,
            this, &MerQmlLiveBenchManager::onAboutToExecuteProject);
}

MerQmlLiveBenchManager::~MerQmlLiveBenchManager()
{
    m_instance = nullptr;
}

MerQmlLiveBenchManager *MerQmlLiveBenchManager::instance()
{
    QTC_CHECK(m_instance);
    return m_instance;
}

void MerQmlLiveBenchManager::startBench()
{
    if (!instance()->m_enabled) {
        warnBenchLocationNotSet();
        return;
    }

    QStringList arguments;
    if (SessionManager::startupProject()) {
        const QString currentProjectDir = SessionManager::startupProject()->projectDirectory().toString();
        arguments.append(currentProjectDir);
    }

    QProcess::startDetached(MerSettings::qmlLiveBenchLocation(), arguments);
}

void MerQmlLiveBenchManager::offerToStartBenchIfNotRunning()
{
    if (!instance()->m_enabled) {
        warnBenchLocationNotSet();
        return;
    }

    auto showDialog = [] {
        QMessageBox *question = new QMessageBox{
                QMessageBox::Question,
                tr("Start QmlLive Bench"),
                tr("<p>QmlLive Bench is not running. Do you want to start it now?</p>"
                   "<p><a href='%1'>Learn more</a> on using QmlLive with Sailfish OS devices.</p>")
                .arg(QLatin1String(Constants::QML_LIVE_HELP_URL)),
                QMessageBox::Yes | QMessageBox::No,
                ICore::mainWindow()};
        question->setAttribute(Qt::WA_DeleteOnClose);
        question->setEscapeButton(QMessageBox::No);
        connect(question, &QMessageBox::finished, instance(), [](int result) {
            if (result == QMessageBox::Yes)
                startBench();
        });
        question->show();
        question->raise();
    };

    Command *ping = new Command;
    ping->arguments = QStringList{QLatin1String(PING_OPTION)};
    ping->expectedExitCodes = {0, 1};
    ping->onFinished = [showDialog](int exitCode) {
        if (exitCode == 1)
            showDialog();
    };

    instance()->enqueueCommand(ping);
}

void MerQmlLiveBenchManager::notifyInferiorRunning(ProjectExplorer::RunControl *rc)
{
    auto merAspect = rc->runConfiguration()->aspect<MerRunConfigurationAspect>();
    if (!merAspect)
        return;
    if (!merAspect->isQmlLiveEnabled())
        return;

    instance()->startProbing(rc);
}

void MerQmlLiveBenchManager::warnBenchLocationNotSet()
{
    QMessageBox *warning = new QMessageBox{
        QMessageBox::Warning,
        tr("Cannot start QmlLive Bench"),
        tr("Location of QmlLive Bench application is either not set or invalid"),
        QMessageBox::Ok,
        ICore::mainWindow()};
    QAbstractButton *configure = warning->addButton(tr("Configure now..."), QMessageBox::AcceptRole);
    connect(configure, &QAbstractButton::clicked,
            [] { ICore::showOptionsDialog(Constants::MER_GENERAL_OPTIONS_ID); });
    warning->setAttribute(Qt::WA_DeleteOnClose);
    warning->setEscapeButton(QMessageBox::Ok);
    warning->show();
    warning->raise();
}

QString MerQmlLiveBenchManager::qmlLiveHostName(const QString &merDeviceName, Utils::Port port)
{
    if (port == Utils::Port(Sfdk::Constants::DEFAULT_QML_LIVE_PORT)) {
        return merDeviceName;
    } else if (port > Utils::Port(Sfdk::Constants::DEFAULT_QML_LIVE_PORT)
            && port < Utils::Port(Sfdk::Constants::DEFAULT_QML_LIVE_PORT + Sfdk::Constants::MAX_PORT_LIST_PORTS)) {
        return merDeviceName + QLatin1Char('_') + QString::number(port.number() - Sfdk::Constants::DEFAULT_QML_LIVE_PORT);
    } else {
        return merDeviceName + QLatin1Char(':') + QString::number(port.number());
    }
}

void MerQmlLiveBenchManager::addHostsToBench(const QString &merDeviceName, const QString &address,
                                            const QList<Utils::Port> &ports)
{
    QTC_ASSERT(!ports.isEmpty(), return);

    if (!m_enabled)
        return;

    QStringList arguments;
    foreach (const Utils::Port &port, ports) {
        arguments.append(QLatin1String(ADD_HOST_OPTION));
        arguments.append(qmlLiveHostName(merDeviceName, port) + QLatin1Char(VALUE_SEPARATOR) +
                         address + QLatin1Char(VALUE_SEPARATOR) +
                         QString::number(port.number()));
    }

    Command *command = new Command;
    command->arguments = arguments;

    enqueueCommand(command);
}

void MerQmlLiveBenchManager::removeHostsFromBench(const QString &merDeviceName, const QList<Utils::Port> &ports)
{
    QTC_ASSERT(!ports.isEmpty(), return);

    if (!m_enabled)
        return;

    QStringList arguments;
    foreach (const Utils::Port &port, ports) {
        arguments.append(QLatin1String(RM_HOST_OPTION));
        arguments.append(qmlLiveHostName(merDeviceName, port));
    }

    Command *command = new Command;
    command->arguments = arguments;

    enqueueCommand(command);
}

void MerQmlLiveBenchManager::letRunningBenchProbeHosts(const QString &merDeviceName, const QList<Utils::Port> &ports)
{
    QTC_ASSERT(!ports.isEmpty(), return);

    if (!m_enabled)
        return;

    QStringList arguments;
    foreach (const Utils::Port &port, ports) {
        arguments.append(QLatin1String(PROBE_HOST_OPTION));
        arguments.append(qmlLiveHostName(merDeviceName, port));
    }

    Command *command = new Command;
    command->arguments = arguments;

    enqueueCommand(command);
}

void MerQmlLiveBenchManager::enqueueCommand(Command *command)
{
    qCDebug(Log::qmlLive) << "ENQUEUE" << command->arguments;
    m_commands.enqueue(command);
    processCommandsQueue();
}

void MerQmlLiveBenchManager::processCommandsQueue()
{
    if (m_commands.isEmpty())
        return;
    if (m_currentCommand)
        return;
    if (!m_enabled) {
        qCWarning(Log::qmlLive) << "Got disabled during command queue processing?";
        return;
    }

    m_currentCommand = m_commands.dequeue();

    m_currentCommand->process = new QProcess{this};
    m_currentCommand->process->setProgram(MerSettings::qmlLiveBenchLocation());
    m_currentCommand->process->setArguments(m_currentCommand->arguments);
    m_currentCommand->process->closeReadChannel(QProcess::StandardOutput);
    m_currentCommand->process->closeWriteChannel();

    auto onError = connect(m_currentCommand->process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        QTC_ASSERT(m_currentCommand, return );
        qCWarning(Log::qmlLive) << "Process error occurred: " << error << m_currentCommand->arguments;
        QByteArray allStdErr(m_currentCommand->process->readAllStandardError());
        qCWarning(Log::qmlLive) << "    allStdErr: " << QString::fromLocal8Bit(allStdErr);

        m_currentCommand->process->deleteLater();
        delete m_currentCommand, m_currentCommand = nullptr;
        processCommandsQueue();
    });

    // Do not let QProcess::errorOccurred and QProcess::finished clash
    connect(m_currentCommand->process, &QProcess::started, this, [this, onError]() {
        m_currentCommand->process->disconnect(onError);
    });

    void (QProcess::*QProcess_finished)(int, QProcess::ExitStatus) = &QProcess::finished;
    connect(m_currentCommand->process, QProcess_finished, this, [this](int exitCode, QProcess::ExitStatus exitStatus) {
        QTC_ASSERT(m_currentCommand, return );
        if (exitStatus != QProcess::NormalExit)
            qCWarning(Log::qmlLive) << "Process crashed: " << m_currentCommand->arguments;
        else if (!m_currentCommand->expectedExitCodes.contains(exitCode))
            qCWarning(Log::qmlLive) << "Process exited with unexpected code: " << exitCode
                                    << m_currentCommand->arguments;
        QByteArray allStdErr(m_currentCommand->process->readAllStandardError());
        if (exitStatus != QProcess::NormalExit || !m_currentCommand->expectedExitCodes.contains(exitCode))
            qCWarning(Log::qmlLive) << "    allStdErr: " << QString::fromLocal8Bit(allStdErr);

        if (m_currentCommand->onFinished)
            m_currentCommand->onFinished(exitCode);

        m_currentCommand->process->deleteLater();
        delete m_currentCommand, m_currentCommand = nullptr;
        processCommandsQueue();
    });

    m_currentCommand->process->start();
}

void MerQmlLiveBenchManager::onBenchLocationChanged()
{
    m_enabled = MerSettings::hasValidQmlLiveBenchLocation();
    if (m_enabled) {
        onStartupProjectChanged(SessionManager::startupProject());
    } else {
        qCWarning(Log::qmlLive) << "QmlLive Bench location invalid or not set. "
            "QmlLive Bench management will not work.";
    }
}

void MerQmlLiveBenchManager::onDeviceAdded(Core::Id id)
{
    if (!m_enabled)
        return;

    IDevice::ConstPtr device = DeviceManager::instance()->find(id);
    auto merDevice = device.dynamicCast<const MerDevice>();
    if (!merDevice)
        return;

    DeviceInfo *info = new DeviceInfo;
    info->name = device->displayName();
    info->ports = merDevice->qmlLivePortsList();
    const QString address = device->sshParameters().host();

    if (info->ports.isEmpty()) {
        qCWarning(Log::qmlLive) << "Not adding QmlLive host with empty port list:" << info->name;
        return;
    }

    m_deviceInfoCache.insert(id, info);

    addHostsToBench(info->name, address, info->ports);
}

void MerQmlLiveBenchManager::onDeviceRemoved(Core::Id id)
{
    if (!m_enabled)
        return;

    if (!m_deviceInfoCache.contains(id))
        return;

    DeviceInfo *cachedInfo = m_deviceInfoCache.take(id);

    removeHostsFromBench(cachedInfo->name, cachedInfo->ports);

    delete cachedInfo;
}

void MerQmlLiveBenchManager::onDeviceListReplaced()
{
    if (!m_enabled)
        return;

    auto difference = [](QList<Utils::Port> ports1, QList<Utils::Port> ports2) {
        std::sort(ports1.begin(), ports1.end());
        std::sort(ports2.begin(), ports2.end());

        QList<Utils::Port> retv;
        std::set_difference(ports1.constBegin(), ports1.constEnd(),
                ports2.constBegin(), ports2.constEnd(), std::back_inserter(retv));

        return retv;
    };

    QSet<Core::Id> currentDevices;

    const int deviceCount = DeviceManager::instance()->deviceCount();
    for (int i = 0; i < deviceCount; ++i) {
        auto merDevice = DeviceManager::instance()->deviceAt(i).dynamicCast<const MerDevice>();
        if (!merDevice)
            continue;

        const Core::Id id = merDevice->id();
        currentDevices.insert(id);

        if (!m_deviceInfoCache.contains(id)) {
            onDeviceAdded(id);
            continue;
        }

        DeviceInfo *cachedInfo = m_deviceInfoCache.take(id);

        DeviceInfo *info = new DeviceInfo;
        info->name = merDevice->displayName();
        info->ports = merDevice->qmlLivePortsList();

        const QList<Utils::Port> removedPorts = difference(cachedInfo->ports, info->ports);
        const QList<Utils::Port> addedPorts = difference(info->ports, cachedInfo->ports);
        const QString address = merDevice->sshParameters().host();

        if (info->name != cachedInfo->name)
            removeHostsFromBench(cachedInfo->name, cachedInfo->ports);
        else if (!removedPorts.isEmpty())
            removeHostsFromBench(info->name, removedPorts);

        if (info->ports.isEmpty()) {
            qCWarning(Log::qmlLive) << "Not adding QmlLive host with empty port list:" << info->name;
            return;
        }

        m_deviceInfoCache.insert(id, info);

        if (info->name != cachedInfo->name)
            addHostsToBench(info->name, address, info->ports);
        else if (!addedPorts.isEmpty())
            addHostsToBench(info->name, address, addedPorts);
    }

    foreach (const Core::Id &id, m_deviceInfoCache.keys()) {
        if (!currentDevices.contains(id))
            onDeviceRemoved(id);
    }
}

void MerQmlLiveBenchManager::onStartupProjectChanged(ProjectExplorer::Project *project)
{
    disconnect(m_activeTargetChangedConnection);
    onActiveTargetChanged(nullptr);

    if (!project)
        return;

    onActiveTargetChanged(project->activeTarget());
    m_activeTargetChangedConnection =
        connect(project, &Project::activeTargetChanged,
                this, &MerQmlLiveBenchManager::onActiveTargetChanged);
}

void MerQmlLiveBenchManager::onActiveTargetChanged(ProjectExplorer::Target *target)
{
    disconnect(m_activeRunConfigurationChangedConnection);
    onActiveRunConfigurationChanged(nullptr);

    if (!target)
        return;

    onActiveRunConfigurationChanged(target->activeRunConfiguration());
    m_activeRunConfigurationChangedConnection =
        connect(target, &Target::activeRunConfigurationChanged,
                this, &MerQmlLiveBenchManager::onActiveRunConfigurationChanged);
}

void MerQmlLiveBenchManager::onActiveRunConfigurationChanged(ProjectExplorer::RunConfiguration *rc)
{
    disconnect(m_qmlLiveEnabledChangedConnection);
    onQmlLiveEnabledChanged(false);

    if (!rc)
        return;
    auto merAspect = rc->aspect<MerRunConfigurationAspect>();
    if (!merAspect)
        return;

    onQmlLiveEnabledChanged(merAspect->isQmlLiveEnabled());
    m_qmlLiveEnabledChangedConnection =
        connect(merAspect, &MerRunConfigurationAspect::qmlLiveEnabledChanged,
                this, &MerQmlLiveBenchManager::onQmlLiveEnabledChanged);
}

void MerQmlLiveBenchManager::onQmlLiveEnabledChanged(bool enabled)
{
    disconnect(m_qmlLiveBenchWorkspaceChangedConnection);
    // Intentionally do not call onQmlLiveBenchWorkspaceChanged(QString{});

    if (!enabled)
        return;

    Project *project = SessionManager::startupProject();
    QTC_ASSERT(project, return);
    Target *target = project->activeTarget();
    QTC_ASSERT(target, return);
    RunConfiguration *rc = target->activeRunConfiguration();
    QTC_ASSERT(rc, return);
    auto merAspect = rc->aspect<MerRunConfigurationAspect>();
    QTC_ASSERT(merAspect, return);

    onQmlLiveBenchWorkspaceChanged(merAspect->qmlLiveBenchWorkspace());
    m_qmlLiveBenchWorkspaceChangedConnection =
        connect(merAspect, &MerRunConfigurationAspect::qmlLiveBenchWorkspaceChanged,
                this, &MerQmlLiveBenchManager::onQmlLiveBenchWorkspaceChanged);
}

void MerQmlLiveBenchManager::onQmlLiveBenchWorkspaceChanged(const QString &benchWorkspace)
{
    if (benchWorkspace.isEmpty() || !QFileInfo(benchWorkspace).isDir())
        return;
    if (!m_enabled)
        return;
    if (!MerSettings::isSyncQmlLiveWorkspaceEnabled())
        return;

    Command *openWorkspace = new Command;
    openWorkspace->arguments = QStringList{QLatin1String(REMOTE_ONLY_OPTION), benchWorkspace};

    enqueueCommand(openWorkspace);
}

void MerQmlLiveBenchManager::onAboutToExecuteProject(ProjectExplorer::RunControl *rc)
{
    auto merAspect = rc->runConfiguration()->aspect<MerRunConfigurationAspect>();
    if (!merAspect)
        return;
    if (!merAspect->isQmlLiveEnabled())
        return;

    offerToStartBenchIfNotRunning();

    startProbing(rc);
}

/*
 * Probe with PROBE_PERIOD_MS milliseconds period for PROBE_TIMEOUT_MS milliseconds. Promptly react
 * to APP_READY_PATTERN appearing in application output, which signalizes qmlliveruntime-sailfish
 * has been initialized.
 */
void MerQmlLiveBenchManager::startProbing(ProjectExplorer::RunControl *rc) { if
    (m_probeTimeouts.contains(rc)) { m_probeTimeouts.value(rc)->start(); return; }

    auto merDevice = rc->device().dynamicCast<const MerDevice>();
    QTC_ASSERT(merDevice, return);

    const QString merDeviceName = merDevice->displayName();
    const QList<Utils::Port> ports = merDevice->qmlLivePortsList();

    if (ports.isEmpty()) {
        qCWarning(Log::qmlLive) << "Not instructing QmlLive Bench to probe host with empty port list:"
                                << merDeviceName;
        return;
    }

    auto rcDestroyed = connect(rc, &QObject::destroyed, this, [this, rc]() {
        delete m_probeTimeouts.take(rc);
    });

    QTimer *timeout = new QTimer(this);
    m_probeTimeouts.insert(rc, timeout);
    timeout->setSingleShot(true);
    timeout->start(PROBE_TIMEOUT_MS);
    connect(timeout, &QTimer::timeout, this, [this, timeout, rc, rcDestroyed, merDeviceName, ports]() {
        letRunningBenchProbeHosts(merDeviceName, ports);
        m_probeTimeouts.take(rc)->deleteLater();
        disconnect(rcDestroyed);
    });

    QTimer *period = new QTimer(timeout);
    period->start(PROBE_PERIOD_MS);
    connect(period, &QTimer::timeout, this, [this, period, merDeviceName, ports]() {
        period->setInterval(PROBE_PERIOD_MS);
        letRunningBenchProbeHosts(merDeviceName, ports);
    });

    auto RunControl_appendMessage = static_cast<
        void (RunControl::*)(RunControl *, const QString &, Utils::OutputFormat)
        >(&RunControl::appendMessageRequested);
    connect(rc, RunControl_appendMessage, period, [period](RunControl *rc, const QString &msg) {
        Q_UNUSED(rc);
        if (msg.contains(QLatin1String(APP_READY_PATTERN)))
            period->setInterval(0);
    });
}

} // Internal
} // QmlLiveBench
