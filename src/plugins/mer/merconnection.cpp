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
#include "virtualboxmanager.h"
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/modemanager.h>
#include <ssh/sshconnection.h>
#include <ssh/sshremoteprocessrunner.h>
#include <utils/qtcassert.h>
#include <projectexplorer/taskhub.h>
#include <projectexplorer/projectexplorer.h>
#include <QAction>
#include <QMessageBox>
#include <QTimer>

namespace Mer {
namespace Internal {

using namespace QSsh;
using namespace ProjectExplorer;

const QSize iconSize = QSize(24, 20);

static int VM_TIMEOUT = 3000;

MerRemoteConnection::MerRemoteConnection(QObject *parent)
    : QObject(parent)
    , m_action(new QAction(this))
    , m_initalized(false)
    , m_visible(false)
    , m_enabled(false)
    , m_connection(0)
    , m_state(Disconnected)
{

}

MerRemoteConnection::~MerRemoteConnection()
{
     if (m_connection) m_connection->deleteLater();
     m_connection = 0;
}

void MerRemoteConnection::setName(const QString &name)
{
    m_name = name;
}

void MerRemoteConnection::setIcon(const QIcon &icon)
{
    m_icon = icon;
}

void MerRemoteConnection::setStartTip(const QString &tip)
{
    m_startTip = tip;
}

void MerRemoteConnection::setStopTip(const QString &tip)
{
    m_stopTip = tip;
}

void MerRemoteConnection::setVisible(bool visible)
{
    m_visible = visible;
}

void MerRemoteConnection::setEnabled(bool enabled)
{
    m_enabled = enabled;
}

void MerRemoteConnection::initialize()
{
    if (m_initalized)
        return;

    m_action->setText(m_name);
    m_action->setIcon(m_icon.pixmap(iconSize));
    m_action->setToolTip(m_startTip);
    connect(m_action, SIGNAL(triggered()), this, SLOT(handleTriggered()));

    Core::Command *command =
            Core::ActionManager::registerAction(m_action, Core::Id(m_name),
                                                Core::Context(Core::Constants::C_GLOBAL));
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setAttribute(Core::Command::CA_UpdateIcon);

    Core::ModeManager::addAction(command->action(), 1);
    m_action->setEnabled(m_enabled);
    m_action->setVisible(m_visible);
    m_initalized = true;
}

bool MerRemoteConnection::isConnected() const
{
    return m_state == Conneted;
}

SshConnectionParameters MerRemoteConnection::parameters() const
{
    if(m_connection) return m_connection->connectionParameters();

    return SshConnectionParameters();
}

void MerRemoteConnection::setSshParameters(const SshConnectionParameters &parameters)
{
    if(m_connection && m_connection->connectionParameters() == parameters) {
        return;
    }

    if (m_connection) m_connection->deleteLater();
    SshConnectionParameters params = parameters;
    params.timeout = 10;
    m_connection = createConnection(params);
}

void MerRemoteConnection::setVirtualMachine(const QString& name)
{
    m_vmName = name;
}

void MerRemoteConnection::update()
{
    QIcon::State state;
    QString toolTip;
    bool enabled = m_enabled;
    switch(m_state) {
        case Conneted:
            state = QIcon::On;
            toolTip = m_stopTip;
            break;
        case Disconnected:
            state = QIcon::Off;
            toolTip = m_startTip;
            break;
        default:
            enabled = false;
            break;
    }

    m_action->setEnabled(enabled);
    m_action->setVisible(m_visible);
    m_action->setToolTip(toolTip);
    m_action->setIcon(m_icon.pixmap(iconSize, QIcon::Normal, state));
}


QSsh::SshConnection* MerRemoteConnection::createConnection(const SshConnectionParameters &params)
{
    SshConnection *connection = new SshConnection(params);
    connect(connection, SIGNAL(connected()), this, SLOT(changeState()));
    connect(connection, SIGNAL(error(QSsh::SshError)), this, SLOT(changeState()));
    connect(connection, SIGNAL(disconnected()), this, SLOT(changeState()));
    return connection;
}

void MerRemoteConnection::handleConnection()
{
    //TODO: this can be removed
    SshConnection *connection = qobject_cast<SshConnection*>(sender());
    if (connection) {
        QTC_ASSERT(connection == m_connection, return);
        changeState();
    }
}

void MerRemoteConnection::changeState(State stateTriger)
{
    QTC_ASSERT(!m_vmName.isEmpty(),return);

    if(stateTriger != NoStateTigger) {
        m_state = stateTriger;
    }

    switch (m_state) {
        case StartingVm:
            if (!VirtualBoxManager::isVirtualMachineRunning(m_vmName)) {
                VirtualBoxManager::startVirtualMachine(m_vmName);
                QTimer::singleShot(VM_TIMEOUT,this,SLOT(changeState()));
            } else {
                m_state = Connecting;
                m_connection->connectToHost();
            }
            break;
        case Connecting:
            if (m_connection->state() == SshConnection::Connected) {
                m_state = Conneted;
            } else if (m_connection->state() == SshConnection::Unconnected) {
                m_state = Disconnected; //broken
                if(m_connection->errorState()!= SshNoError) {
                    createConnectionErrorTask(m_vmName,m_connection->errorString());
                }
            } else {
                QTimer::singleShot(VM_TIMEOUT,this,SLOT(changeState()));
            }
            break;
        case Conneted:
            break;
        case Disconneting:
             if (m_connection->state() == SshConnection::Connected) {
                 m_connection->disconnectFromHost();
             } else if (m_connection->state() == SshConnection::Unconnected) {
                 VirtualBoxManager::shutVirtualMachine(m_vmName);
                 QSsh::SshConnectionParameters sshParams = m_connection->connectionParameters();
                 sshParams.userName = QLatin1String("root");
                 QSsh::SshRemoteProcessRunner *runner = new QSsh::SshRemoteProcessRunner(m_connection);
                 connect(runner, SIGNAL(processClosed(int)), runner, SLOT(deleteLater()));
                 runner->run("shutdown now", sshParams);
                 QTimer::singleShot(VM_TIMEOUT,this,SLOT(changeState()));
                 m_state = ClosingVm;
             } else {
                 QTimer::singleShot(VM_TIMEOUT,this,SLOT(changeState()));
             }
            break;
        case ClosingVm:
            if (!VirtualBoxManager::isVirtualMachineRunning(m_vmName)) {
                m_state = Disconnected;
            } else {
                qWarning() << "Could not close virtual machine" << m_vmName;
                m_state = Disconnected;
            }
            break;
        case Disconnected:
            break;
    }
    update();
}

void MerRemoteConnection::handleTriggered()
{
    if(!m_connection) return;

    if (m_state == Disconnected) {
        changeState(StartingVm);
    } else if(m_state == Conneted) {
        changeState(Disconneting);
    }
}

void  MerRemoteConnection::createConnectionErrorTask(const QString &vmName, const QString &error)
{
    TaskHub* th = ProjectExplorerPlugin::instance()->taskHub();
    th->addTask(Task(Task::Error,
                     tr("%1: %2").arg(vmName, error),
                     Utils::FileName() /* filename */,
                     -1 /* linenumber */,
                     Core::Id(Constants::MER_TASKHUB_CATEGORY)));
}

bool MerRemoteConnection::promptToStart(const QString& vm)
{
    const QMessageBox::StandardButton response =
        QMessageBox::question(0, tr("Start Virtual Machine"),
                              tr("Virtual Machine '%1' is not running! Please start the "
                                 "Virtual Machine and retry after the Virtual Machine is "
                                 "running.\n\n"
                                 "Start Virtual Machine now?").arg(vm),
                              QMessageBox::No | QMessageBox::Yes, QMessageBox::Yes);
    if (response == QMessageBox::Yes) {
        return true;
    }
    return false;
}

} // Internal
} // Mer
