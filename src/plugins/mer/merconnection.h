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

#ifndef MERCONNECTION_H
#define MERCONNECTION_H

#include <ssh/sshconnection.h>
#include <qglobal.h>
#include <QObject>
#include <QIcon>
#include <coreplugin/id.h>

QT_BEGIN_NAMESPACE
class QAction;
QT_END_NAMESPACE

namespace QSsh {
class SshConnection;
class SshConnectionParameters;
}

namespace Mer {
namespace Internal {

class MerConnection : public QObject
{
    Q_OBJECT
public:
    enum State {
        NoStateTrigger = -1,
        Disconnected,
        StartingVm,
        TryConnect,
        Connecting,
        Connected,
        Disconnecting,
        ClosingVm
    };

    explicit MerConnection(QObject *parent = 0);
    ~MerConnection();

    void setVirtualMachine(const QString &virtualMachine);
    void setSshParameters(const QSsh::SshConnectionParameters &sshParameters);
    void setProbeTimeout(int timeout);
    void setHeadless(bool headless);
    QSsh::SshConnectionParameters sshParameters() const;
    QString virtualMachine() const;
    State state() const;
    bool isConnected() const;
    bool hasError() const;
    QString errorString() const;
    void connectTo();
    void disconnectFrom();
    void tryConnectTo();
    void setupConnection();
signals:
    void stateChanged();
    void hasErrorChanged(bool hasError);

private slots:
    void changeState(State state = NoStateTrigger);

private:
    QSsh::SshConnection* createConnection(const QSsh::SshConnectionParameters &sshParameters);

private:
    bool m_connectionInitialized;
    QSsh::SshConnection* m_connection;
    QString m_vmName;
    State m_state;
    QSsh::SshConnectionParameters m_params;
    int m_vmStartupTimeOut;
    int m_vmCloseTimeOut;
    int m_probeTimeout;
    bool m_reportError;
    bool m_headless;
    QString m_errorString;
};

}
}
#endif
