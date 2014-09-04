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
#include <ssh/sshconnection.h>
#include <ssh/sshremoteprocessrunner.h>
#include <utils/qtcassert.h>
#include <projectexplorer/taskhub.h>
#include <projectexplorer/projectexplorer.h>
#include <QTimer>

namespace Mer {
namespace Internal {

using namespace QSsh;
using namespace ProjectExplorer;

MerConnection::MerConnection(QObject *parent)
    : QObject(parent)
    , m_connectionInitialized(false)
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

void MerConnection::setProbeTimeout(int timeout)
{
    m_probeTimeout = timeout;
}

void MerConnection::setHeadless(bool headless)
{
    m_headless = headless;
}

bool MerConnection::isConnected() const
{
    return m_state == Connected;
}

SshConnectionParameters MerConnection::sshParameters() const
{
    return m_params;
}

void MerConnection::setVirtualMachine(const QString &virtualMachine)
{
    if(m_vmName != virtualMachine) {
        m_vmName = virtualMachine;
        m_connectionInitialized = false;
    }
}

void MerConnection::setSshParameters(const SshConnectionParameters &sshParameters)
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

MerConnection::State MerConnection::state() const
{
    return m_state;
}

bool MerConnection::hasError() const
{
    return !m_errorString.isEmpty();
}

QString MerConnection::errorString() const
{
    return m_errorString;
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
            if (hasError()) {
              m_errorString.clear();
              emit hasErrorChanged(false);
            }
        } else if (m_connection->state() == SshConnection::Unconnected) {
            m_state = Disconnected; //broken
            if (m_connection->errorState() != SshNoError && m_reportError) {
                m_errorString = m_connection->errorString();
                emit hasErrorChanged(true);
            }
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
    emit stateChanged();
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

void MerConnection::disconnectFrom()
{
    if (m_state == Connected) {
        changeState(Disconnecting);
    }
}

} // Internal
} // Mer
