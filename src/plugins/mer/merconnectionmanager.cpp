/****************************************************************************
**
** Copyright (C) 2012 - 2014 Jolla Ltd.
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

#include <coreplugin/actionmanager/command.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icontext.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/modemanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <projectexplorer/session.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <ssh/sshconnection.h>
#include <utils/qtcassert.h>

#include <QAction>
#include <QIcon>

using namespace ProjectExplorer;

namespace Mer {
namespace Internal {

class MerConnectionAction : public QObject
{
    Q_OBJECT
public:
    explicit MerConnectionAction(QObject *parent = 0);
    ~MerConnectionAction();

    void setId(const Core::Id &id) { m_id = id; }
    void setName(const QString &name) { m_name = name; }
    void setIconOff(const QIcon &icon) { m_iconOff = icon; }
    void setIconOn(const QIcon &icon) { m_iconOn = icon; }
    void setStartTip(const QString &tip) { m_startTip = tip; }
    void setStopTip(const QString &tip) { m_stopTip = tip; }
    void setConnectingTip(const QString &tip) { m_connTip = tip; }
    void setDisconnectingTip(const QString &tip) { m_discoTip = tip; }
    void setStartingTip(const QString &tip) { m_startingTip = tip; }
    void setClosingTip(const QString &tip) { m_closingTip = tip; }
    void setVisible(bool visible) { m_visible = visible; }
    void setEnabled(bool enabled) { m_enabled = enabled; }

    void initialize();

    void setConnection(MerConnection *connection);

public slots:
    void update();

private slots:
    void handleTriggered();

private:
    QPointer<MerConnection> m_connection;
    Core::Id m_id;
    QAction *m_action;
    QIcon m_iconOff;
    QIcon m_iconOn;
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

MerConnectionAction::MerConnectionAction(QObject *parent)
    : QObject(parent)
    , m_connection(0)
    , m_action(new QAction(this))
    , m_uiInitalized(false)
    , m_visible(false)
    , m_enabled(false)
{
    setEnabled(false);
}

MerConnectionAction::~MerConnectionAction()
{
}

void MerConnectionAction::initialize()
{
    if (m_uiInitalized)
        return;

    m_action->setText(m_name);
    m_action->setIcon(m_iconOff);
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

    m_uiInitalized = true;
}

void MerConnectionAction::setConnection(MerConnection *connection)
{
    if (m_connection == connection)
        return;

    if (m_connection)
        m_connection->disconnect(this);

    m_connection = connection;

    if (m_connection)
        connect(m_connection, SIGNAL(stateChanged()), this, SLOT(update()));

    update();
}

void MerConnectionAction::update()
{
    if (!m_connection) {
        setEnabled(false);
        return;
    }

    QIcon::State state = QIcon::Off;
    QString toolTip;
    bool enabled = m_enabled;

    switch (m_connection->state()) {
    case MerConnection::Connected:
        state = QIcon::On;
        toolTip = m_stopTip;
        break;
    case MerConnection::Disconnected:
        toolTip = m_startTip;
        break;
    case MerConnection::StartingVm:
        enabled = false;
        state = QIcon::On;
        toolTip = m_startingTip;
        break;
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
    case MerConnection::Error:
        Core::MessageManager::write(tr("Error connecting to \"%1\" virtual machine: %2")
            .arg(m_connection->virtualMachine())
            .arg(m_connection->errorString()),
            Core::MessageManager::Flash);
        toolTip = m_startTip;
        break;
    default:
        qWarning() << "MerConnection::update() - unknown state";
        break;
    }

    m_action->setEnabled(enabled);
    m_action->setVisible(m_visible);
    m_action->setToolTip(toolTip);
    m_action->setIcon(state == QIcon::Off ? m_iconOff : m_iconOn);
}

void MerConnectionAction::handleTriggered()
{
    QTC_ASSERT(m_connection, return);

    if (m_connection->state() == MerConnection::Disconnected) {
        m_connection->connectTo();
    } else if (m_connection->state() == MerConnection::Connected) {
        m_connection->disconnectFrom();
    } else if (m_connection->state() == MerConnection::Error) {
        m_connection->connectTo();
    }
}

MerConnectionManager *MerConnectionManager::m_instance = 0;
ProjectExplorer::Project *MerConnectionManager::m_project = 0;

MerConnectionManager::MerConnectionManager():
    m_emulatorAction(new MerConnectionAction(this)),
    m_sdkAction(new MerConnectionAction(this))
{
    QIcon emuIconOff(QLatin1String(":/mer/images/emulator-run.png"));
    QIcon emuIconOn(QLatin1String(":/mer/images/emulator-stop.png"));
    QIcon sdkIconOff(QLatin1String(":/mer/images/sdk-run.png"));
    QIcon sdkIconOn(QLatin1String(":/mer/images/sdk-stop.png"));

    m_emulatorAction->setName(tr("Emulator"));
    m_emulatorAction->setId(Constants::MER_EMULATOR_CONNECTON_ACTION_ID);
    m_emulatorAction->setIconOff(emuIconOff);
    m_emulatorAction->setIconOn(emuIconOn);
    m_emulatorAction->setStartTip(tr("Start Emulator"));
    m_emulatorAction->setStopTip(tr("Stop Emulator"));
    m_emulatorAction->setConnectingTip(tr("Connecting..."));
    m_emulatorAction->setDisconnectingTip(tr("Disconnecting..."));
    m_emulatorAction->setClosingTip(tr("Closing Emulator"));
    m_emulatorAction->setStartingTip(tr("Starting Emulator"));
    m_emulatorAction->initialize();

    m_sdkAction->setName(tr("MerSdk"));
    m_sdkAction->setId(Constants::MER_SDK_CONNECTON_ACTION_ID);
    m_sdkAction->setIconOff(sdkIconOff);
    m_sdkAction->setIconOn(sdkIconOn);
    m_sdkAction->setStartTip(tr("Start SDK"));
    m_sdkAction->setStopTip(tr("Stop SDK"));
    m_sdkAction->setConnectingTip(tr("Connecting..."));
    m_sdkAction->setDisconnectingTip(tr("Disconnecting..."));
    m_sdkAction->setClosingTip(tr("Closing SDK"));
    m_sdkAction->setStartingTip(tr("Starting SDK"));
    m_sdkAction->initialize();

    connect(KitManager::instance(), SIGNAL(kitUpdated(ProjectExplorer::Kit*)),
            SLOT(handleKitUpdated(ProjectExplorer::Kit*)));

    connect(SessionManager::instance(), SIGNAL(startupProjectChanged(ProjectExplorer::Project*)),
            SLOT(handleStartupProjectChanged(ProjectExplorer::Project*)));
    connect(MerSdkManager::instance(), SIGNAL(sdksUpdated()), this, SLOT(update()));
    connect(DeviceManager::instance(), SIGNAL(updated()), SLOT(update()));

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

void MerConnectionManager::handleKitUpdated(Kit *kit)
{
    if (MerSdkManager::isMerKit(kit))
        update();
}

void MerConnectionManager::handleStartupProjectChanged(ProjectExplorer::Project *project)
{
    if (m_project) {
        disconnect(m_project, SIGNAL(addedTarget(ProjectExplorer::Target*)),
                   this, SLOT(handleTargetAdded(ProjectExplorer::Target*)));
        disconnect(m_project, SIGNAL(removedTarget(ProjectExplorer::Target*)),
                   this, SLOT(handleTargetRemoved(ProjectExplorer::Target*)));
        disconnect(m_project, SIGNAL(activeTargetChanged(ProjectExplorer::Target*)),
                   this, SLOT(update()));
    }

    m_project = project;

    if (m_project) {
        connect(m_project, SIGNAL(addedTarget(ProjectExplorer::Target*)),
                SLOT(handleTargetAdded(ProjectExplorer::Target*)));
        connect(m_project, SIGNAL(removedTarget(ProjectExplorer::Target*)),
                SLOT(handleTargetRemoved(ProjectExplorer::Target*)));
        connect(m_project, SIGNAL(activeTargetChanged(ProjectExplorer::Target*)),
                SLOT(update()));
    }

    update();
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
                    m_sdkAction->setConnection(sdk->connection());
                }
            }

            if (MerSdkManager::hasMerDevice(t->kit())) {
                if (t == activeTarget) {
                    bool hasEmulatorDevice =
                            DeviceTypeKitInformation::deviceTypeId(t->kit()) == Core::Id(Constants::MER_DEVICE_TYPE_I486);
                    if (hasEmulatorDevice) {
                          emulatorRemoteButtonVisible = true;
                          emulatorRemoteButtonEnabled = true;
                          const IDevice::ConstPtr device = DeviceKitInformation::device(t->kit());
                          const MerEmulatorDevice* emu = static_cast<const MerEmulatorDevice*>(device.data());
                          m_emulatorAction->setConnection(emu->connection());
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

}
}

#include "merconnectionmanager.moc"
