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

#include <coreplugin/id.h>
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

    void setId(const Core::Id &id);
    void setName(const QString &name);
    void setIcon(const QIcon &icon);
    void setStartTip(const QString &tip);
    void setStopTip(const QString &tip);
    void setConnectingTip(const QString &tip);
    void setDisconnectingTip(const QString &tip);
    void setStartingTip(const QString &tip);
    void setClosingTip(const QString &tip);
    void setVisible(bool visible);
    void setEnabled(bool enabled);
    void setTaskCategory(Core::Id id);
    void setVirtualMachine(const QString vm);
    void setConnectionParameters(const QSsh::SshConnectionParameters &sshParameters);
    void setProbeTimeout(int timeout);
    void setHeadless(bool headless);
    QSsh::SshConnectionParameters sshParameters() const;
    QString virtualMachine() const;
    bool isConnected() const;
    void initialize();
    void update();
    void connectTo();
    void disconnectFrom();
    void tryConnectTo();
    void setupConnection();
    static void createConnectionErrorTask(const QString &vmName, const QString &error, Core::Id category);
    static void removeConnectionErrorTask(Core::Id category);

private slots:
    void handleTriggered();
    void changeState(State state = NoStateTrigger);

private:
    QSsh::SshConnection* createConnection(const QSsh::SshConnectionParameters &sshParameters);

private:
    Core::Id m_id;
    QAction *m_action;
    QIcon m_icon;
    QString m_name;
    QString m_startTip;
    QString m_stopTip;
    QString m_connTip;
    QString m_discoTip;
    QString m_closingTip;
    QString m_startingTip;
    bool m_uiInitalized;
    bool m_connectionInitialized;
    bool m_visible;
    bool m_enabled;
    QSsh::SshConnection* m_connection;
    QString m_vmName;
    State m_state;
    Core::Id m_taskId;
    QSsh::SshConnectionParameters m_params;
    int m_vmStartupTimeOut;
    int m_vmCloseTimeOut;
    int m_probeTimeout;
    bool m_reportError;
    bool m_headless;
};

}
}
#endif
