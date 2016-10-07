/****************************************************************************
**
** Copyright (C) 2016 Jolla Ltd.
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

#include <QCoreApplication>
#include <QMessageBox>
#include <QProcess>
#include <QTimer>

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
#include "merlogging.h"
#include "merrunconfigurationaspect.h"
#include "mersettings.h"

using namespace Core;
using namespace ProjectExplorer;

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
const int PROBE_LATEST_MS = 2500;
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

    connect(ProjectExplorerPlugin::instance(), &ProjectExplorerPlugin::runControlStarted,
            this, &MerQmlLiveBenchManager::onRunControlStarted);
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
    if (!instance()->m_enabled)
        return;

    QStringList arguments;
    if (SessionManager::startupProject()) {
        const QString currentProjectDir = SessionManager::startupProject()->projectDirectory().toString();
        arguments.append(currentProjectDir);
    }

    QProcess::startDetached(MerSettings::qmlLiveBenchLocation(), arguments);
}

void MerQmlLiveBenchManager::offerToStartBenchIfNotRunning()
{
    if (!instance()->m_enabled)
        return;

    auto showDialog = [] {
        QMessageBox *question = new QMessageBox{
                QMessageBox::Question,
                tr("Start QmlLive Bench"),
                tr("QmlLive Bench is not running. Do you want to start it now?"),
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

QString MerQmlLiveBenchManager::qmlLiveHostName(const QString &merDeviceName, int port)
{
    if (port == Constants::DEFAULT_QML_LIVE_PORT) {
        return merDeviceName;
    } else if (port > Constants::DEFAULT_QML_LIVE_PORT
            && port < (Constants::DEFAULT_QML_LIVE_PORT + Constants::MAX_QML_LIVE_PORTS)) {
        return merDeviceName + QLatin1Char('_') + QString::number(port - Constants::DEFAULT_QML_LIVE_PORT);
    } else {
        return merDeviceName + QLatin1Char(':') + QString::number(port);
    }
}

void MerQmlLiveBenchManager::addHostsToBench(const QString &merDeviceName, const QString &address,
                                            const QSet<int> &ports)
{
    QTC_ASSERT(!ports.isEmpty(), return);

    if (!m_enabled)
        return;

    QStringList arguments;
    foreach (int port, ports) {
        arguments.append(QLatin1String(ADD_HOST_OPTION));
        arguments.append(qmlLiveHostName(merDeviceName, port) + QLatin1Char(VALUE_SEPARATOR) +
                         address + QLatin1Char(VALUE_SEPARATOR) +
                         QString::number(port));
    }

    Command *command = new Command;
    command->arguments = arguments;

    enqueueCommand(command);
}

void MerQmlLiveBenchManager::removeHostsFromBench(const QString &merDeviceName, const QSet<int> &ports)
{
    QTC_ASSERT(!ports.isEmpty(), return);

    if (!m_enabled)
        return;

    QStringList arguments;
    foreach (int port, ports) {
        arguments.append(QLatin1String(RM_HOST_OPTION));
        arguments.append(qmlLiveHostName(merDeviceName, port));
    }

    Command *command = new Command;
    command->arguments = arguments;

    enqueueCommand(command);
}

void MerQmlLiveBenchManager::letRunningBenchProbeHosts(const QString &merDeviceName, const QSet<int> &ports)
{
    QTC_ASSERT(!ports.isEmpty(), return);

    if (!m_enabled)
        return;

    QStringList arguments;
    foreach (int port, ports) {
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

    void (QProcess::*QProcess_error)(QProcess::ProcessError) = &QProcess::error;
    auto onError = connect(m_currentCommand->process, QProcess_error, this, [this](QProcess::ProcessError error) {
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
    info->ports = merDevice->qmlLivePortsSet();
    const QString address = device->sshParameters().host;

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
        info->ports = merDevice->qmlLivePortsSet();

        const QSet<int> removedPorts = cachedInfo->ports - info->ports;
        const QSet<int> addedPorts = info->ports - cachedInfo->ports;
        const QString address = merDevice->sshParameters().host;

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
    auto merAspect = rc->extraAspect<MerRunConfigurationAspect>();
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
    auto merAspect = rc->extraAspect<MerRunConfigurationAspect>();
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

void MerQmlLiveBenchManager::onRunControlStarted(ProjectExplorer::RunControl *rc)
{
    auto merAspect = rc->runConfiguration()->extraAspect<MerRunConfigurationAspect>();
    if (!merAspect)
        return;
    if (!merAspect->isQmlLiveEnabled())
        return;

    offerToStartBenchIfNotRunning();

    // Remaining part is to call letRunningBenchProbeHosts() when app become ready

    auto merDevice = rc->device().dynamicCast<const MerDevice>();
    QTC_ASSERT(merDevice, return);

    const QString merDeviceName = merDevice->displayName();
    const QSet<int> ports = merDevice->qmlLivePortsSet();

    if (ports.isEmpty()) {
        qCWarning(Log::qmlLive) << "Not instructing QmlLive Bench to probe host with empty port list:"
                                << merDeviceName;
        return;
    }

    // Wait for APP_READY_PATTERN to appear in application output, signalizing
    // qmlliveruntime-sailfish has been initialized. Do not wait longer than
    // PROBE_LATEST_MS milliseconds.

    QPointer<QObject> guard = new QObject(this);
    auto probe = [this, guard, merDeviceName, ports] {
        Q_ASSERT(guard);
        delete guard;
        letRunningBenchProbeHosts(merDeviceName, ports);
    };

    auto RunControl_appendMessage = static_cast<
        void (RunControl::*)(RunControl *, const QString &, Utils::OutputFormat)
        >(&RunControl::appendMessage);
    connect(rc, RunControl_appendMessage, guard.data(), [probe](RunControl *rc, const QString &msg) {
        Q_UNUSED(rc);
        if (msg.contains(QLatin1String(APP_READY_PATTERN)))
            probe();
    });

    QTimer::singleShot(PROBE_LATEST_MS, guard.data(), [probe] {
        qCWarning(Log::qmlLive) << "Timeout waiting for application to become ready.";
        probe();
    });
}

} // Internal
} // QmlLiveBench
