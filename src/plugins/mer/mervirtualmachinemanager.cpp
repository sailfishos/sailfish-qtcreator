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

#include "mervirtualmachinemanager.h"
#include "virtualboxmanager.h"

#include <ssh/sshconnectionmanager.h>
#include <ssh/sshremoteprocessrunner.h>
#include <utils/qtcassert.h>

#include <QTimer>

namespace Mer {
namespace Internal {

MerVirtualMachineManager *MerVirtualMachineManager::m_remoteManager = 0;
const int MerVirtualMachineManager::m_connectionRetries = 10;
const int MerVirtualMachineManager::m_initialConnectionTimeout = 3; // in seconds. Increases with ecah attempt.

MerVirtualMachineManager::MerVirtualMachineManager(QObject *parent)
    : QObject(parent)
{
}

MerVirtualMachineManager::~MerVirtualMachineManager()
{
    QSsh::SshConnectionManager &mgr = QSsh::SshConnectionManager::instance();
    foreach (QSsh::SshConnection *connection, m_sshConnections)
        mgr.releaseConnection(connection);
    QList<QSsh::SshConnection *> connections = m_sshConnectionsInProgress.keys();
    foreach (QSsh::SshConnection *connection, connections)
        mgr.releaseConnection(connection);
}

MerVirtualMachineManager *MerVirtualMachineManager::instance()
{
    if (m_remoteManager)
        return m_remoteManager;

    m_remoteManager = new MerVirtualMachineManager;
    return m_remoteManager;
}

bool MerVirtualMachineManager::isRemoteConnected(const QString &vmName) const
{
    // We check if we have a valid ssh connection
    return m_sshConnections.contains(vmName);
}

bool MerVirtualMachineManager::isRemoteRunning(const QString &vmName) const
{
    return VirtualBoxManager::isVirtualMachineRunning(vmName);
}

bool MerVirtualMachineManager::isRemoteRegistered(const QString &vmName) const
{
    return VirtualBoxManager::isVirtualMachineRegistered(vmName);
}

bool MerVirtualMachineManager::startRemote(const QString &vmName,
                                           const QSsh::SshConnectionParameters &params)
{
    // Check if we already initiated a connection to this emulator
    if (m_sshConnections.contains(vmName))
        return true;
    // Is the virtual machine running? Maybe we already initiated a connection.
    if (VirtualBoxManager::isVirtualMachineRunning(vmName)) {
        QList<QPair<QString, int> > connInProgessEmulators = m_sshConnectionsInProgress.values();
        typedef QList<QPair<QString, int> >::const_iterator EmulatorsInProgressIterator;
        EmulatorsInProgressIterator itEnd = connInProgessEmulators.end();
        for (EmulatorsInProgressIterator it = connInProgessEmulators.begin(); it != itEnd; ++it) {
            if ((*it).first == vmName)
                return false;
        }
    }

    // Start emulator and connect to it. Timeout = m_connectionTimeout s
    // and Max tries = m_connectionRetries
    VirtualBoxManager::startVirtualMachine(vmName);
    QSsh::SshConnectionParameters sshParams = params;
    sshParams.timeout = m_initialConnectionTimeout;
    QSsh::SshConnectionManager &mgr = QSsh::SshConnectionManager::instance();
    mgr.forceNewConnection(sshParams);
    return connectToRemote(sshParams, vmName, 0);
}

bool MerVirtualMachineManager::connectToRemote(const QString &vmName,
                                               const QSsh::SshConnectionParameters &params)
{
    // Check if we already initiated a connection to this emulator
    if (m_sshConnections.contains(vmName))
        return true;
    // Is the virtual machine running? Maybe we already initiated a connection.
    if (VirtualBoxManager::isVirtualMachineRunning(vmName)) {
        QList<QPair<QString, int> > connInProgessEmulators = m_sshConnectionsInProgress.values();
        typedef QList<QPair<QString, int> >::const_iterator EmulatorsInProgressIterator;
        EmulatorsInProgressIterator itEnd = connInProgessEmulators.end();
        for (EmulatorsInProgressIterator it = connInProgessEmulators.begin(); it != itEnd; ++it) {
            if ((*it).first == vmName)
                return false;
        }
    }

    QSsh::SshConnectionParameters sshParams = params;
    sshParams.timeout = 10000;
    QSsh::SshConnectionManager &mgr = QSsh::SshConnectionManager::instance();
    QSsh::SshConnection *connection = mgr.acquireConnection(sshParams);
    if (connection->state() == QSsh::SshConnection::Unconnected)
        connection->connectToHost();

    connect(connection, SIGNAL(error(QSsh::SshError)), SLOT(onError(QSsh::SshError)));
    connect(connection, SIGNAL(disconnected()), SLOT(onDisconnected()));

    if (connection->state() == QSsh::SshConnection::Connected) {
        m_sshConnections.insert(vmName, connection);
        return true;
    } else {
        connect(connection, SIGNAL(connected()), SLOT(onConnected()));
        m_sshConnectionsInProgress.insert(connection, qMakePair(vmName, 0));
    }
    return false;
}

void MerVirtualMachineManager::stopRemote(const QString &vmName)
{
    VirtualBoxManager::shutVirtualMachine(vmName);
}

void MerVirtualMachineManager::onError(QSsh::SshError error)
{
    QSsh::SshConnection *connection = qobject_cast<QSsh::SshConnection *>(sender());
    if (connection && m_sshConnectionsInProgress.contains(connection)) {
        QPair<QString, int> val = m_sshConnectionsInProgress.value(connection);
        const QString &vmName = val.first;
        const int attempt = val.second;
        if (error == QSsh::SshTimeoutError && attempt < m_connectionRetries
                && VirtualBoxManager::isVirtualMachineRunning(vmName)) {
            QSsh::SshConnectionParameters sshParams = connection->connectionParameters();
            sshParams.timeout = (attempt + 1) * m_initialConnectionTimeout;
            QSsh::SshConnectionManager::instance().releaseConnection(connection);
            m_sshConnectionsInProgress.remove(connection);
            connectToRemote(sshParams, vmName, attempt + 1);
        } else if (error == QSsh::SshSocketError && attempt != -1) {
            m_sshConnectionRetries.push_back(connection);
            QTimer::singleShot(m_initialConnectionTimeout * 1000, this, SLOT(retryOnSocketError()));
        } else if (error == QSsh::SshAuthenticationError && attempt < m_connectionRetries) {
            // TODO: Sometimes we get the wrong error "Server rejected key."
            // We need to keep trying. (BUG)
            const QSsh::SshConnectionParameters sshParams = connection->connectionParameters();
            QSsh::SshConnectionManager::instance().releaseConnection(connection);
            m_sshConnectionsInProgress.remove(connection);
            connectToRemote(sshParams, vmName, attempt + 1);
        } else if (attempt != -1) {
            emit this->error(vmName, connection->errorString());
            m_sshConnectionsInProgress.remove(connection);
            QSsh::SshConnectionManager::instance().releaseConnection(connection);
            emit connectionChanged(val.first, false);
        }
    }
}

void MerVirtualMachineManager::onConnected()
{
    QSsh::SshConnection *connection = qobject_cast<QSsh::SshConnection *>(sender());
    if (connection && m_sshConnectionsInProgress.contains(connection)) {
        const QString vmName = m_sshConnectionsInProgress.take(connection).first;
        m_sshConnections.insert(vmName, connection);
        emit connectionChanged(vmName, true);
    }
}

void MerVirtualMachineManager::onDisconnected()
{
    QSsh::SshConnection *connection = qobject_cast<QSsh::SshConnection *>(sender());
    if (connection) {
        QString vmName = m_sshConnectionsInProgress.take(connection).first;
        typedef QHash<QString, QSsh::SshConnection *>::const_iterator SshConnectionIterator;
        SshConnectionIterator itEnd = m_sshConnections.end();
        for (SshConnectionIterator it = m_sshConnections.begin(); it != itEnd; ++it) {
            if (it.value() == connection) {
                vmName = it.key();
                m_sshConnections.remove(vmName);
                break;
            }
        }
        QSsh::SshConnectionManager::instance().releaseConnection(connection);
        emit connectionChanged(vmName, false);
    }
}

void MerVirtualMachineManager::onRemoteProcessClosed()
{
    QSsh::SshRemoteProcessRunner *runner = qobject_cast<QSsh::SshRemoteProcessRunner *>(sender());
    delete runner;
}

void MerVirtualMachineManager::retryOnSocketError()
{
    QTC_ASSERT(!m_sshConnectionRetries.isEmpty(), return);
    QSsh::SshConnection *connection = m_sshConnectionRetries.takeFirst();
    if (connection && m_sshConnectionsInProgress.contains(connection)) {
        QPair<QString, int> val = m_sshConnectionsInProgress.value(connection);
        const QString &vmName = val.first;
        const int attempt = val.second;
        // retry if the VM is still running
        if (attempt < m_connectionRetries
                && VirtualBoxManager::isVirtualMachineRunning(vmName)) {
            m_sshConnectionsInProgress.insert(connection, qMakePair(vmName, attempt + 1));
            connection->connectToHost();
        } else {
            emit error(vmName, connection->errorString());
            m_sshConnectionsInProgress.remove(connection);
            QSsh::SshConnectionManager::instance().releaseConnection(connection);
            emit connectionChanged(val.first, false);
        }
    }
}

bool MerVirtualMachineManager::connectToRemote(const QSsh::SshConnectionParameters &params,
                                               const QString &vmName, int attempt)
{
    QSsh::SshConnectionManager &mgr = QSsh::SshConnectionManager::instance();
    QSsh::SshConnection *connection = mgr.acquireConnection(params);
    if (connection->state() == QSsh::SshConnection::Unconnected)
        connection->connectToHost();

    connect(connection, SIGNAL(error(QSsh::SshError)), SLOT(onError(QSsh::SshError)));
    connect(connection, SIGNAL(disconnected()), SLOT(onDisconnected()));

    if (connection->state() == QSsh::SshConnection::Connected) {
        m_sshConnections.insert(vmName, connection);
        return true;
    } else {
        connect(connection, SIGNAL(connected()), SLOT(onConnected()));
        m_sshConnectionsInProgress.insert(connection, qMakePair(vmName, attempt));
    }
    return false;
}

} // Internal
} // Mer
