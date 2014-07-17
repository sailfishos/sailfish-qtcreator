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

#include "merconnection.h"
#include "merconstants.h"
#include "mervirtualboxmanager.h"
#include "merconnectionmanager.h"
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icontext.h>
#include <coreplugin/modemanager.h>
#include <ssh/sshconnection.h>
#include <ssh/sshremoteprocessrunner.h>
#include <utils/qtcassert.h>
#include <projectexplorer/taskhub.h>
#include <projectexplorer/projectexplorer.h>
#include <QAction>
#include <QTimer>

namespace Mer {
namespace Internal {

using namespace QSsh;
using namespace ProjectExplorer;

const QSize iconSize = QSize(24, 20);

MerConnection::MerConnection(QObject *parent)
    : QObject(parent)
    , m_action(new QAction(this))
    , m_uiInitalized(false)
    , m_connectionInitialized(false)
    , m_visible(false)
    , m_enabled(false)
    , m_connection(0)
    , m_state(Disconnected)
    , m_vmStartupTimeOut(3000)
    , m_vmCloseTimeOut(3000)
    , m_probeTimeout(3000)
    , m_reportError(true)
    , m_headless(false)
{

}

MerConnection::~MerConnection()
{
    if (m_connection)
        m_connection->deleteLater();
    m_connection = 0;
}

void MerConnection::setId(const Core::Id &id)
{
    m_id = id;
}

void MerConnection::setName(const QString &name)
{
    m_name = name;
}

void MerConnection::setIcon(const QIcon &icon)
{
    m_icon = icon;
}

void MerConnection::setStartTip(const QString &tip)
{
    m_startTip = tip;
}

void MerConnection::setStopTip(const QString &tip)
{
    m_stopTip = tip;
}

void MerConnection::setConnectingTip(const QString &tip)
{
    m_connTip = tip;
}

void MerConnection::setDisconnectingTip(const QString &tip)
{
    m_discoTip = tip;
}

void MerConnection::setClosingTip(const QString &tip)
{
    m_closingTip = tip;
}

void MerConnection::setStartingTip(const QString &tip)
{
    m_startingTip = tip;
}

void MerConnection::setVisible(bool visible)
{
    m_visible = visible;
}

void MerConnection::setEnabled(bool enabled)
{
    m_enabled = enabled;
}

void MerConnection::setTaskCategory(Core::Id id)
{
    m_taskId = id;
}

void MerConnection::setProbeTimeout(int timeout)
{
    m_probeTimeout = timeout;
}

void MerConnection::setHeadless(bool headless)
{
    m_headless = headless;
}

void MerConnection::initialize()
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
    m_uiInitalized = true;

    TaskHub::addCategory(m_taskId, tr("Virtual Machine Error"));
}

bool MerConnection::isConnected() const
{
    return m_state == Connected;
}

SshConnectionParameters MerConnection::sshParameters() const
{
    return m_params;
}

void MerConnection::setVirtualMachine(const QString vm)
{
    if(m_vmName != vm) {
        m_vmName = vm;
        m_connectionInitialized = false;
    }
}

void MerConnection::setConnectionParameters(const SshConnectionParameters &sshParameters)
{
    if(m_params != sshParameters) {
        m_params = sshParameters;
        m_connectionInitialized = false;
    }
}

void MerConnection::setupConnection()
{
    if (m_connectionInitialized)
        return;

    if (m_connection) {
        m_connection->disconnect();
        m_connection->deleteLater();
    }

    m_connection = createConnection(m_params);
    m_state = Disconnected;
    m_connectionInitialized = true;
}

QString MerConnection::virtualMachine() const
{
    return m_vmName;
}

void MerConnection::update()
{
    QIcon::State state = QIcon::Off;
    QString toolTip;
    bool enabled = m_enabled;

    switch (m_state) {
    case Connected:
        state = QIcon::On;
        toolTip = m_stopTip;
        break;
    case Disconnected:
        toolTip = m_startTip;
        break;
    case StartingVm:
        enabled = false;
        state = QIcon::On;
        toolTip = m_startingTip;
        break;
    case TryConnect:
    case Connecting:
        enabled = false;
        state = QIcon::On;
        toolTip = m_connTip;
        break;
    case Disconnecting:
        enabled = false;
        toolTip = m_discoTip;
        break;
    case ClosingVm:
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


QSsh::SshConnection* MerConnection::createConnection(const SshConnectionParameters &params)
{
    SshConnection *connection = new SshConnection(params);
    connect(connection, SIGNAL(connected()), this, SLOT(changeState()));
    connect(connection, SIGNAL(error(QSsh::SshError)), this, SLOT(changeState()));
    connect(connection, SIGNAL(disconnected()), this, SLOT(changeState()));
    return connection;
}

void MerConnection::changeState(State stateTrigger)
{
    QTC_ASSERT(!m_vmName.isEmpty(), return);
    QTC_ASSERT(m_connectionInitialized, return);

    if (stateTrigger != NoStateTrigger)
        m_state = stateTrigger;

    switch (m_state) {
    case StartingVm:
        if (MerVirtualBoxManager::isVirtualMachineRunning(m_vmName)) {
            m_state = Connecting;
            m_reportError = true;
            m_connection->connectToHost();
        } else {
            MerVirtualBoxManager::startVirtualMachine(m_vmName,m_headless);
            QTimer::singleShot(m_vmStartupTimeOut, this, SLOT(changeState()));
        }
        break;
    case TryConnect:
        if (m_connection->state() == SshConnection::Unconnected) {
            m_state = Connecting;
            m_reportError = false;
            m_connection->connectToHost();
            QTimer::singleShot(m_probeTimeout, this, SLOT(changeState()));
        }
        break;
    case Connecting:
        if (m_connection->state() == SshConnection::Connected) {
            m_state = Connected;
            removeConnectionErrorTask(m_taskId);
        } else if (m_connection->state() == SshConnection::Unconnected) {
            m_state = Disconnected; //broken
            if (m_connection->errorState() != SshNoError && m_reportError)
                createConnectionErrorTask(m_vmName, m_connection->errorString(),m_taskId);
        } else if (m_connection->state() == SshConnection::Connecting) {
            m_connection->disconnectFromHost();
            m_state = Disconnected;
        }
        break;
    case Connected:
        if(m_connection->state() == SshConnection::Unconnected) {
            m_state = Disconnected;
        } else if (m_connection->state() == SshConnection::Connecting) {
            m_state = Connecting;
        } //Connected
        break;
    case Disconnecting:
        if (m_connection->state() == SshConnection::Connected ||
                m_connection->state() == SshConnection::Connecting) {
            m_connection->disconnectFromHost();
        } else if (m_connection->state() == SshConnection::Unconnected) {
            QSsh::SshConnectionParameters sshParams = m_connection->connectionParameters();
            QSsh::SshRemoteProcessRunner *runner = new QSsh::SshRemoteProcessRunner(m_connection);
            connect(runner, SIGNAL(processClosed(int)), runner, SLOT(deleteLater()));
            runner->run("sdk-shutdown", sshParams);
            QTimer::singleShot(m_vmCloseTimeOut, this, SLOT(changeState()));
            m_state = ClosingVm;
        }
        break;
    case ClosingVm:
        if (MerVirtualBoxManager::isVirtualMachineRunning(m_vmName)) {
            qWarning() << "Could not close virtual machine" << m_vmName;
            qWarning() << "Trying force..";
            MerVirtualBoxManager::shutVirtualMachine(m_vmName);
        }
        m_state = Disconnected;
        break;
    case Disconnected:
        break;
    default:
        break;
    }
    update();
}

void MerConnection::connectTo()
{
    if (!m_connection)
        return;

    if (m_state == Disconnected)
        changeState(StartingVm);
}

//connects only if vm running;
void MerConnection::tryConnectTo()
{
    if (!MerVirtualBoxManager::isVirtualMachineRunning(m_vmName))
        return;

    if (m_state == Disconnected) {
        changeState(TryConnect);
    }
}

void MerConnection::handleTriggered()
{
    if (!m_connection)
        return;

    if (m_state == Disconnected) {
        changeState(StartingVm);
    } else if (m_state == Connected) {
        changeState(Disconnecting);
    }
}


void MerConnection::disconnectFrom()
{
    if (m_state == Connected) {
        changeState(Disconnecting);
    }
}


void  MerConnection::createConnectionErrorTask(const QString &vmName, const QString &error, Core::Id category)
{
    TaskHub::clearTasks(category);
    TaskHub::addTask(Task(Task::Error,
                          tr("%1: %2").arg(vmName, error),
                          Utils::FileName() /* filename */,
                          -1 /* linenumber */,
                          category));
}

void  MerConnection::removeConnectionErrorTask(Core::Id category)
{
    TaskHub::clearTasks(category);
}

} // Internal
} // Mer
