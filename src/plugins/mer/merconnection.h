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

class MerRemoteConnection : public QObject
{
    Q_OBJECT
public:
    enum State {
        NoStateTrigger = -1,
        Disconnected,
        StartingVm,
        Connecting,
        Connected,
        Disconneting,
        ClosingVm
    };

    explicit MerRemoteConnection(QObject *parent = 0);
    ~MerRemoteConnection();

    void setId(const Core::Id &id);
    void setName(const QString &name);
    void setIcon(const QIcon &icon);
    void setStartTip(const QString &tip);
    void setStopTip(const QString &tip);
    void setVisible(bool visible);
    void setEnabled(bool enabled);
    void setConnectionParameters(const QString &virtualMachine, const QSsh::SshConnectionParameters &sshParameters);
    QSsh::SshConnectionParameters sshParameters() const;
    QString virtualMachine() const;
    bool isConnected() const;
    void initialize();
    void update();
    void connectTo();

    static const int m_connectionTimeOut = 77; // seconds

private slots:
    void handleTriggered();
    void handleConnection();
    void changeState(State state = NoStateTrigger);

private:
    QSsh::SshConnection* createConnection(const QSsh::SshConnectionParameters &params);
    void createConnectionErrorTask(const QString &vmName, const QString &error);

private:
    Core::Id m_id;
    QAction *m_action;
    QIcon m_icon;
    QString m_name;
    QString m_startTip;
    QString m_stopTip;
    bool m_initalized;
    bool m_visible;
    bool m_enabled;
    QSsh::SshConnection* m_connection;
    QString m_vmName;
    State m_state;
};

}
}
#endif
