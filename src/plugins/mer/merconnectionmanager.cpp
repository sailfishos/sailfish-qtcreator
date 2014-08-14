/****************************************************************************
**
** Copyright (C) 2012 - 2013 Jolla Ltd.
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

#include "merconnectionmanager.h"
#include "merconnection.h"
#include "mersdkmanager.h"
#include "merconstants.h"
#include "mervirtualboxmanager.h"
#include "meremulatordevice.h"
#include "mersdkkitinformation.h"
#include "merconnectionprompt.h"

#include <coreplugin/actionmanager/command.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icontext.h>
#include <coreplugin/modemanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <projectexplorer/session.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/taskhub.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/buildmanager.h>
#include <ssh/sshconnection.h>
#include <utils/qtcassert.h>

#include <QAction>
#include <QIcon>

using namespace ProjectExplorer;

namespace Mer {
namespace Internal {

const QSize iconSize = QSize(24, 20);

class MerConnectionAction : public QObject
{
    Q_OBJECT
public:
    explicit MerConnectionAction(MerConnection *connection, QObject *parent = 0);
    ~MerConnectionAction();

    void setId(const Core::Id &id) { m_id = id; }
    void setName(const QString &name) { m_name = name; }
    void setIcon(const QIcon &icon) { m_icon = icon; }
    void setTaskCategory(Core::Id id) { m_taskId = id; }
    void setStartTip(const QString &tip) { m_startTip = tip; }
    void setStopTip(const QString &tip) { m_stopTip = tip; }
    void setConnectingTip(const QString &tip) { m_connTip = tip; }
    void setDisconnectingTip(const QString &tip) { m_discoTip = tip; }
    void setStartingTip(const QString &tip) { m_startingTip = tip; }
    void setClosingTip(const QString &tip) { m_closingTip = tip; }
    void setVisible(bool visible) { m_visible = visible; }
    void setEnabled(bool enabled) { m_enabled = enabled; }

    void initialize();

public slots:
    void update();

private slots:
    void handleTriggered();
    void onHasErrorChanged(bool hasError);

private:
    QPointer<MerConnection> m_connection;
    Core::Id m_id;
    QAction *m_action;
    QIcon m_icon;
    Core::Id m_taskId;
    QString m_name;
    QString m_startTip;
    QString m_stopTip;
    QString m_connTip;
    QString m_discoTip;
    QString m_closingTip;
    QString m_startingTip;
    bool m_uiInitalized;
    bool m_visible;
    bool m_enabled;
};

MerConnectionAction::MerConnectionAction(MerConnection *connection, QObject *parent)
    : QObject(parent)
    , m_connection(connection)
    , m_action(new QAction(this))
    , m_uiInitalized(false)
    , m_visible(false)
    , m_enabled(false)
{
    connect(m_connection, SIGNAL(stateChanged()), this, SLOT(update()));
    connect(m_connection, SIGNAL(hasErrorChanged(bool)), this, SLOT(onHasErrorChanged(bool)));
}

MerConnectionAction::~MerConnectionAction()
{
}

void MerConnectionAction::initialize()
{
    if (m_uiInitalized)
        return;

    m_action->setText(m_name);
    m_action->setIcon(m_icon.pixmap(iconSize));
    m_action->setToolTip(m_startTip);
    connect(m_action, SIGNAL(triggered()), this, SLOT(handleTriggered()));

    Core::Command *command =
            Core::ActionManager::registerAction(m_action, m_id,
                                                Core::Context(Core::Constants::C_GLOBAL));
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setAttribute(Core::Command::CA_UpdateIcon);

    Core::ModeManager::addAction(command->action(), 1);
    m_action->setEnabled(m_enabled);
    m_action->setVisible(m_visible);

    TaskHub::addCategory(m_taskId, tr("Virtual Machine Error"));

    m_uiInitalized = true;
}

void MerConnectionAction::update()
{
    QIcon::State state = QIcon::Off;
    QString toolTip;
    bool enabled = m_enabled;

    switch (m_connection->state()) {
    case MerConnection::Connected:
        state = QIcon::On;
        toolTip = m_stopTip;
        MerConnectionManager::removeConnectionErrorTask(m_taskId);
        break;
    case MerConnection::Disconnected:
        toolTip = m_startTip;
        break;
    case MerConnection::StartingVm:
        enabled = false;
        state = QIcon::On;
        toolTip = m_startingTip;
        break;
    case MerConnection::TryConnect:
    case MerConnection::Connecting:
        enabled = false;
        state = QIcon::On;
        toolTip = m_connTip;
        break;
    case MerConnection::Disconnecting:
        enabled = false;
        toolTip = m_discoTip;
        break;
    case MerConnection::ClosingVm:
        enabled = false;
        toolTip = m_closingTip;
        break;
    default:
        qWarning() << "MerConnection::update() - unknown state";
        break;
    }

    m_action->setEnabled(enabled);
    m_action->setVisible(m_visible);
    m_action->setToolTip(toolTip);
    m_action->setIcon(m_icon.pixmap(iconSize, QIcon::Normal, state));
}

void MerConnectionAction::handleTriggered()
{
    if (m_connection->state() == MerConnection::Disconnected) {
        m_connection->connectTo();
    } else if (m_connection->state() == MerConnection::Connected) {
        m_connection->disconnectFrom();
    }
}

void MerConnectionAction::onHasErrorChanged(bool hasError)
{
    // All task (including those created from e.g. merdeploysteps.cpp) are
    // cleared on state change to Connected in update()

    if (hasError) {
        MerConnectionManager::createConnectionErrorTask(m_connection->virtualMachine(),
                m_connection->errorString(), m_taskId);
    }
}

MerConnectionManager *MerConnectionManager::m_instance = 0;
ProjectExplorer::Project *MerConnectionManager::m_project = 0;

MerConnectionManager::MerConnectionManager():
    m_emulatorConnection(new MerConnection()),
    m_emulatorAction(new MerConnectionAction(m_emulatorConnection.data())),
    m_sdkConnection(new MerConnection()),
    m_sdkAction(new MerConnectionAction(m_sdkConnection.data()))
{
    QIcon emuIcon;
    emuIcon.addFile(QLatin1String(":/mer/images/emulator-run.png"));
    emuIcon.addFile(QLatin1String(":/mer/images/emulator-stop.png"), QSize(), QIcon::Normal,
                    QIcon::On);
    QIcon sdkIcon;
    sdkIcon.addFile(QLatin1String(":/mer/images/sdk-run.png"));
    sdkIcon.addFile(QLatin1String(":/mer/images/sdk-stop.png"), QSize(), QIcon::Normal, QIcon::On);

    m_emulatorAction->setName(tr("Emulator"));
    m_emulatorAction->setId(Constants::MER_EMULATOR_CONNECTON_ACTION_ID);
    m_emulatorAction->setIcon(emuIcon);
    m_emulatorAction->setStartTip(tr("Start Emulator"));
    m_emulatorAction->setStopTip(tr("Stop Emulator"));
    m_emulatorAction->setConnectingTip(tr("Connecting..."));
    m_emulatorAction->setDisconnectingTip(tr("Disconnecting..."));
    m_emulatorAction->setClosingTip(tr("Closing Emulator"));
    m_emulatorAction->setStartingTip(tr("Starting Emulator"));
    m_emulatorAction->setTaskCategory(Core::Id(Constants::MER_TASKHUB_EMULATOR_CATEGORY));
    m_emulatorAction->initialize();
    m_emulatorConnection->setProbeTimeout(1000);

    m_sdkAction->setName(tr("MerSdk"));
    m_sdkAction->setId(Constants::MER_SDK_CONNECTON_ACTION_ID);
    m_sdkAction->setIcon(sdkIcon);
    m_sdkAction->setStartTip(tr("Start SDK"));
    m_sdkAction->setStopTip(tr("Stop SDK"));
    m_sdkAction->setConnectingTip(tr("Connecting..."));
    m_sdkAction->setDisconnectingTip(tr("Disconnecting..."));
    m_sdkAction->setClosingTip(tr("Closing SDK"));
    m_sdkAction->setStartingTip(tr("Starting SDK"));
    m_sdkAction->setTaskCategory(Core::Id(Constants::MER_TASKHUB_SDK_CATEGORY));
    m_sdkAction->initialize();
    m_sdkConnection->setProbeTimeout(1000);

    connect(KitManager::instance(), SIGNAL(kitUpdated(ProjectExplorer::Kit*)),
            SLOT(handleKitUpdated(ProjectExplorer::Kit*)));

    connect(SessionManager::instance(), SIGNAL(startupProjectChanged(ProjectExplorer::Project*)),
            SLOT(handleStartupProjectChanged(ProjectExplorer::Project*)));
    connect(MerSdkManager::instance(), SIGNAL(sdksUpdated()), this, SLOT(update()));
    connect(DeviceManager::instance(), SIGNAL(updated()), SLOT(update()));

    connect(BuildManager::instance(), SIGNAL(buildStateChanged(ProjectExplorer::Project*)),
            this, SLOT(handleBuildStateChanged(ProjectExplorer::Project*)));

    m_instance = this;
}

MerConnectionManager::~MerConnectionManager()
{
    m_instance = 0;
}

MerConnectionManager* MerConnectionManager::instance()
{
    Q_ASSERT(m_instance);
    return m_instance;
}

bool MerConnectionManager::isConnected(const QString &vmName) const
{
    if(m_emulatorConnection->virtualMachine() == vmName) {
        return m_emulatorConnection->isConnected();
    }else if(m_sdkConnection->virtualMachine() == vmName) {
        return m_sdkConnection->isConnected();
    }
    return false;
}

QSsh::SshConnectionParameters MerConnectionManager::parameters(const MerSdk *sdk)
{
    QSsh::SshConnectionParameters params;
    params.userName = sdk->userName();
    params.host = sdk->host();
    params.port = sdk->sshPort();
    params.privateKeyFile = sdk->privateKeyFile();
    params.timeout = sdk->timeout();
    return params;
}

void MerConnectionManager::handleKitUpdated(Kit *kit)
{
    if (MerSdkManager::isMerKit(kit))
        update();
}

void MerConnectionManager::handleStartupProjectChanged(ProjectExplorer::Project *project)
{
    if (project != m_project && project) {
        // handle all target related changes, add, remove, etc...
        connect(project, SIGNAL(addedTarget(ProjectExplorer::Target*)),
                SLOT(handleTargetAdded(ProjectExplorer::Target*)));
        connect(project, SIGNAL(removedTarget(ProjectExplorer::Target*)),
                SLOT(handleTargetRemoved(ProjectExplorer::Target*)));
        connect(project, SIGNAL(activeTargetChanged(ProjectExplorer::Target*)),
                SLOT(update()));
    }

    if (m_project) {
        disconnect(m_project, SIGNAL(addedTarget(ProjectExplorer::Target*)),
                   this, SLOT(handleTargetAdded(ProjectExplorer::Target*)));
        disconnect(m_project, SIGNAL(removedTarget(ProjectExplorer::Target*)),
                   this, SLOT(handleTargetRemoved(ProjectExplorer::Target*)));
        disconnect(m_project, SIGNAL(activeTargetChanged(ProjectExplorer::Target*)),
                   this, SLOT(update()));
    }

    m_project = project;
    update();
    m_emulatorConnection->tryConnectTo();
    m_sdkConnection->tryConnectTo();
}

void MerConnectionManager::handleTargetAdded(Target *target)
{
    if (target && MerSdkManager::isMerKit(target->kit()))
        update();
}

void MerConnectionManager::handleTargetRemoved(Target *target)
{
    if (target && MerSdkManager::isMerKit(target->kit()))
        update();
}

void MerConnectionManager::handleBuildStateChanged(Project* project)
{
     Target* target = project->activeTarget();
     if (target && target->kit() && MerSdkManager::isMerKit(target->kit())) {
         if (BuildManager::isBuilding(project)) {
             if(m_sdkConnection->isConnected()) return;
             const QString vm = m_sdkConnection->virtualMachine();
             QTC_ASSERT(!vm.isEmpty(),return);
             if (!MerVirtualBoxManager::isVirtualMachineRunning(vm)) {
                 MerConnectionPrompt *connectioPrompt = new MerConnectionPrompt(vm);
                 connectioPrompt->prompt(MerConnectionPrompt::Start);
             } else {
                 m_sdkConnection->connectTo();
             }
         }
     }
}

void MerConnectionManager::update()
{
    bool sdkRemoteButtonEnabled = false;
    bool emulatorRemoteButtonEnabled = false;
    bool sdkRemoteButtonVisible = false;
    bool emulatorRemoteButtonVisible = false;

    const Project* const p = SessionManager::startupProject();
    if (p) {
        const Target* const activeTarget = p->activeTarget();
        const QList<Target *> targets =  p->targets();

        foreach (const Target *t , targets) {
            if (MerSdkManager::isMerKit(t->kit())) {
                sdkRemoteButtonVisible = true;
                if (t == activeTarget) {
                    sdkRemoteButtonEnabled = true;
                    MerSdk* sdk = MerSdkKitInformation::sdk(t->kit());
                    QTC_ASSERT(sdk, continue);
                    QString sdkName = sdk->virtualMachineName();
                    QTC_ASSERT(!sdkName.isEmpty(), continue);
                    m_sdkConnection->setVirtualMachine(sdkName);
                    m_sdkConnection->setHeadless(sdk->isHeadless());
                    m_sdkConnection->setConnectionParameters(parameters(sdk));
                    m_sdkConnection->setupConnection();
                }
            }

            if (MerSdkManager::hasMerDevice(t->kit())) {
                emulatorRemoteButtonVisible = true;
                if (t == activeTarget) {
                    emulatorRemoteButtonEnabled =
                            DeviceTypeKitInformation::deviceTypeId(t->kit()) == Core::Id(Constants::MER_DEVICE_TYPE_I486);
                    if(emulatorRemoteButtonEnabled) {
                          const IDevice::ConstPtr device = DeviceKitInformation::device(t->kit());
                          const MerEmulatorDevice* emu = static_cast<const MerEmulatorDevice*>(device.data());
                          const QString &emulatorName = emu->virtualMachine();
                          m_emulatorConnection->setVirtualMachine(emulatorName);
                          m_emulatorConnection->setConnectionParameters(device->sshParameters());
                          m_emulatorConnection->setupConnection();
                    }
                }
            }
        }
    }

    m_emulatorAction->setEnabled(emulatorRemoteButtonEnabled);
    m_emulatorAction->setVisible(emulatorRemoteButtonVisible);
    m_sdkAction->setEnabled(sdkRemoteButtonEnabled);
    m_sdkAction->setVisible(sdkRemoteButtonVisible);
    m_emulatorAction->update();
    m_sdkAction->update();
}

QString MerConnectionManager::testConnection(const QSsh::SshConnectionParameters &params) const
{
    QSsh::SshConnectionParameters p = params;
    QSsh::SshConnection connection(p);
    QEventLoop loop;
    connect(&connection, SIGNAL(connected()), &loop, SLOT(quit()));
    connect(&connection, SIGNAL(error(QSsh::SshError)), &loop, SLOT(quit()));
    connect(&connection, SIGNAL(disconnected()), &loop, SLOT(quit()));
    connection.connectToHost();
    loop.exec();
    QString result;
    if (connection.errorState() != QSsh::SshNoError)
        result = connection.errorString();
    else
        result = tr("Connected");
    return result;
}

void MerConnectionManager::connectTo(const QString &vmName)
{
    if(m_emulatorConnection->virtualMachine() == vmName) {
        m_emulatorConnection->connectTo();
    }else if(m_sdkConnection->virtualMachine() == vmName) {
        m_sdkConnection->connectTo();
    }
}

void MerConnectionManager::disconnectFrom(const QString &vmName)
{
    if(m_emulatorConnection->virtualMachine() == vmName) {
        m_emulatorConnection->disconnectFrom();
    }else if(m_sdkConnection->virtualMachine() == vmName) {
        m_sdkConnection->disconnectFrom();
    }
}

void MerConnectionManager::createConnectionErrorTask(const QString &vmName, const QString &error, Core::Id category)
{
    TaskHub::clearTasks(category);
    TaskHub::addTask(Task(Task::Error,
                          tr("%1: %2").arg(vmName, error),
                          Utils::FileName() /* filename */,
                          -1 /* linenumber */,
                          category));
}

void MerConnectionManager::removeConnectionErrorTask(Core::Id category)
{
    TaskHub::clearTasks(category);
}

}
}

#include "merconnectionmanager.moc"
