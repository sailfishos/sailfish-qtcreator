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

#ifndef MERCONNECTION_H
#define MERCONNECTION_H

#include <ssh/sshconnection.h>

#include <QBasicTimer>
#include <QObject>
#include <QPointer>
#include <QtGlobal>

QT_BEGIN_NAMESPACE
class QMessageBox;
QT_END_NAMESPACE

namespace Mer {
namespace Internal {

class MerConnectionRemoteShutdownProcess;

class MerConnection : public QObject
{
    Q_OBJECT
    Q_ENUMS(State)

    enum VmState {
        VmOff,
        VmAskBeforeStarting,
        VmStarting,
        VmStartingError,
        VmRunning,
        VmSoftClosing,
        VmHardClosing,
        VmZombie,
    };

    enum SshState {
        SshNotConnected,
        SshConnecting,
        SshConnectingError,
        SshConnected,
        SshDisconnecting,
        SshDisconnected,
        SshConnectionLost,
    };

public:
    enum State {
        Disconnected,
        StartingVm,
        Connecting,
        Error,
        Connected,
        Disconnecting,
        ClosingVm
    };

    enum ConnectOption {
        NoConnectOption = 0x00,
        AskStartVm = 0x01,
    };
    Q_DECLARE_FLAGS(ConnectOptions, ConnectOption)

    explicit MerConnection(QObject *parent = 0);
    ~MerConnection() override;

    void setVirtualMachine(const QString &virtualMachine);
    void setSshParameters(const QSsh::SshConnectionParameters &sshParameters);
    void setHeadless(bool headless);

    QSsh::SshConnectionParameters sshParameters() const;
    bool isHeadless() const;
    QString virtualMachine() const;

    State state() const;
    QString errorString() const;

    bool isAutoConnectEnabled() const;
    void setAutoConnectEnabled(bool autoConnectEnabled);

    bool isVirtualMachineOff(bool *runningHeadless = 0) const;
    bool lockDown(bool lockDown);

    static QStringList usedVirtualMachines();

public slots:
    void refresh();
    void connectTo(Mer::Internal::MerConnection::ConnectOptions options = NoConnectOption);
    void disconnectFrom();

signals:
    void stateChanged();
    void virtualMachineChanged();
    void virtualMachineOffChanged(bool vmOff);
    void lockDownFailed();

protected:
    void timerEvent(QTimerEvent *event) override;

private:
    void scheduleReset();
    void reset();

    // state machine
    void updateState();
    void vmStmTransition(VmState toState, const char *event);
    bool vmStmExiting();
    bool vmStmEntering();
    void vmStmExec();
    bool vmStmStep();
    void sshStmTransition(SshState toState, const char *event);
    bool sshStmExiting();
    bool sshStmEntering();
    void sshStmExec();
    bool sshStmStep();

    void createConnection();
    void vmWantFastPollState(bool want);
    void vmPollState();
    void sshTryConnect();

    // dialogs
    void openAlreadyConnectingWarningBox();
    void openAlreadyDisconnectingWarningBox();
    void openVmNotRegisteredWarningBox();
    void openStartVmQuestionBox();
    void openResetVmQuestionBox();
    void openCloseVmQuestionBox();
    void openUnableToCloseVmWarningBox();
    void openRetrySshConnectionQuestionBox();
    void openRetryLockDownQuestionBox();
    void deleteMessageBox(QPointer<QMessageBox> &messageBox);

    static const char *str(State state);
    static const char *str(VmState vmState);
    static const char *str(SshState sshState);

private slots:
    void vmStmScheduleExec();
    void sshStmScheduleExec();
    void onSshConnected();
    void onSshDisconnected();
    void onSshError(QSsh::SshError error);
    void onRemoteShutdownProcessFinished();

private:
    static QMap<QString, int> s_usedVmNames;

    QPointer<QSsh::SshConnection> m_connection;
    QString m_vmName;
    QSsh::SshConnectionParameters m_params;
    bool m_headless;

    // state
    State m_state;
    QString m_errorString;
    VmState m_vmState;
    bool m_vmStartedOutside;
    SshState m_sshState;

    // on-transition flags
    bool m_vmStmTransition;
    bool m_sshStmTransition;

    // state machine inputs (notice the difference in handling
    // m_lockDownRequested compared to m_{dis,}connectRequested!)
    bool m_lockDownRequested;
    bool m_lockDownFailed;
    bool m_autoConnectEnabled;
    bool m_connectRequested;
    bool m_disconnectRequested;
    bool m_connectLaterRequested;
    ConnectOptions m_connectOptions;
    bool m_cachedVmRunning;
    bool m_cachedSshConnected;
    QSsh::SshError m_cachedSshError;
    QString m_cachedSshErrorString;

    // timeout timers
    QBasicTimer m_vmStartingTimeoutTimer;
    QBasicTimer m_vmSoftClosingTimeoutTimer;
    QBasicTimer m_vmHardClosingTimeoutTimer;

    // background task timers
    int m_vmWantFastPollState;
    QBasicTimer m_vmStatePollTimer;
    QBasicTimer m_sshTryConnectTimer;

    // state machine idle execution
    QBasicTimer m_vmStmExecTimer;
    QBasicTimer m_sshStmExecTimer;

    // auto invoke reset after properties are changed
    QBasicTimer m_resetTimer;

    // dialogs
    QPointer<QMessageBox> m_startVmQuestionBox;
    QPointer<QMessageBox> m_resetVmQuestionBox;
    QPointer<QMessageBox> m_closeVmQuestionBox;
    QPointer<QMessageBox> m_unableToCloseVmWarningBox;
    QPointer<QMessageBox> m_retrySshConnectionQuestionBox;
    QPointer<QMessageBox> m_retryLockDownQuestionBox;

    QPointer<MerConnectionRemoteShutdownProcess> m_remoteShutdownProcess;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(MerConnection::ConnectOptions)

}
}
#endif
