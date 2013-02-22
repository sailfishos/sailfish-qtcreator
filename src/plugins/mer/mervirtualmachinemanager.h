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

#ifndef MERVIRTUALMACHINEMANAGER_H
#define MERVIRTUALMACHINEMANAGER_H

#include <ssh/sshconnection.h>

#include <QObject>

namespace Mer {
namespace Internal {

class MerVirtualMachineManager : public QObject
{
    Q_OBJECT
public:
    static MerVirtualMachineManager *instance();

    bool isRemoteConnected(const QString &vmName) const;
    bool isRemoteRunning(const QString &vmName) const;
    bool isRemoteRegistered(const QString &vmName) const;
    bool startRemote(const QString &vmName, const QSsh::SshConnectionParameters &params);
    bool connectToRemote(const QString &vmName, const QSsh::SshConnectionParameters &params);
    void stopRemote(const QString &vmName);
    void stopRemote(const QString &vmName, const QSsh::SshConnectionParameters &params);

signals:
    void connectionChanged(const QString &vmName, bool isConnected);
    void error(const QString &vmName, const QString &error);

private slots:
    void onError(QSsh::SshError);
    void onConnected();
    void onDisconnected();
    void onRemoteProcessClosed();
    void retryOnSocketError();

private:
    MerVirtualMachineManager(QObject *parent = 0);
    ~MerVirtualMachineManager();

    bool connectToRemote(const QSsh::SshConnectionParameters &params, const QString &vmName,
                         int attempt);

private:
    static MerVirtualMachineManager *m_remoteManager;
    QHash<QString, QSsh::SshConnection *> m_sshConnections;
    QHash<QSsh::SshConnection *, QPair<QString,int> > m_sshConnectionsInProgress;
    QList<QSsh::SshConnection *> m_sshConnectionRetries;
    static const int m_connectionRetries;
    static const int m_initialConnectionTimeout;
};

} // Internal
} // Mer

#endif // MERVIRTUALMACHINEMANAGER_H
