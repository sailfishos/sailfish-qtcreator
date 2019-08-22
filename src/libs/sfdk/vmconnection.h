/****************************************************************************
**
** Copyright (C) 2012-2019 Jolla Ltd.
** Copyright (C) 2019 Open Mobile Platform LLC.
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

#pragma once

#include "sfdkglobal.h"

#include <ssh/sshconnection.h>

#include <QBasicTimer>
#include <QObject>
#include <QPointer>
#include <QTime>
#include <QtGlobal>

#include <functional>

namespace Sfdk {

class VmConnectionRemoteShutdownProcess;

class SFDK_EXPORT VmConnection : public QObject
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
        Block = 0x02,
    };
    Q_DECLARE_FLAGS(ConnectOptions, ConnectOption)

    enum Synchronization {
        Asynchronous,
        Synchronous
    };

    class Ui;

    explicit VmConnection(QObject *parent = 0);
    ~VmConnection() override;

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

    bool isVirtualMachineOff(bool *runningHeadless = 0, bool *startedOutside = 0) const;
    bool lockDown(bool lockDown);

    static QStringList usedVirtualMachines();

    template<typename ConcreteUi>
    static void registerUi()
    {
        Q_ASSERT(!s_uiCreator);
        s_uiCreator = [](VmConnection *parent) { return new ConcreteUi(parent); };
    }

public slots:
    void refresh(Sfdk::VmConnection::Synchronization synchronization = Asynchronous);
    bool connectTo(Sfdk::VmConnection::ConnectOptions options = NoConnectOption);
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
    void vmPollState(Synchronization synchronization);
    void waitForVmPollStateFinish();
    void sshTryConnect();

    static const char *str(State state);
    static const char *str(VmState vmState);
    static const char *str(SshState sshState);

private slots:
    void vmStmScheduleExec();
    void sshStmScheduleExec();
    void onSshConnected();
    void onSshDisconnected();
    void onSshErrorOccured();
    void onRemoteShutdownProcessFinished();

private:
    using UiCreator = std::function<Ui *(VmConnection *)>;
    static UiCreator s_uiCreator;
    static QMap<QString, int> s_usedVmNames;

    QPointer<QSsh::SshConnection> m_connection;
    QString m_vmName;
    QSsh::SshConnectionParameters m_params;
    bool m_headless;

    // state
    State m_state;
    QString m_errorString;
    VmState m_vmState;
    QTime m_vmStateEntryTime;
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
    bool m_cachedVmExists;
    bool m_cachedVmRunning;
    bool m_cachedSshConnected;
    bool m_cachedSshErrorOccured;
    QString m_cachedSshErrorString;
    QPointer<QSsh::SshConnection> m_cachedSshErrorOrigin;

    // timeout timers
    QBasicTimer m_vmStartingTimeoutTimer;
    QBasicTimer m_vmSoftClosingTimeoutTimer;
    QBasicTimer m_vmHardClosingTimeoutTimer;

    // background task timers
    int m_vmWantFastPollState;
    QBasicTimer m_vmStatePollTimer;
    bool m_pollingVmState;
    QBasicTimer m_sshTryConnectTimer;

    // state machine idle execution
    QBasicTimer m_vmStmExecTimer;
    QBasicTimer m_sshStmExecTimer;

    // auto invoke reset after properties are changed
    QBasicTimer m_resetTimer;

    Ui *m_ui;
    QPointer<VmConnectionRemoteShutdownProcess> m_remoteShutdownProcess;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(VmConnection::ConnectOptions)

class SFDK_EXPORT VmConnection::Ui : public QObject
{
    Q_OBJECT

public:
    enum Warning {
        AlreadyConnecting,
        AlreadyDisconnecting,
        UnableToCloseVm,
        VmNotRegistered,
    };

    enum Question {
        StartVm,
        ResetVm,
        CloseVm,
        CancelConnecting,
        CancelLockingDown,
    };

    enum QuestionStatus {
        NotAsked,
        Asked,
        Yes,
        No,
    };

    using OnStatusChanged = void (VmConnection::*)();

    using QObject::QObject;

    virtual void warn(Warning which) = 0;
    virtual void dismissWarning(Warning which) = 0;

    virtual bool shouldAsk(Question which) const = 0;
    virtual void ask(Question which, OnStatusChanged onStatusChanged) = 0;
    virtual void dismissQuestion(Question which) = 0;
    virtual QuestionStatus status(Question which) const = 0;

    void ask(Question which, OnStatusChanged onStatusChanged,
            std::function<void()> ifYes, std::function<void()> ifNo);

protected:
    VmConnection *connection() const { return static_cast<VmConnection *>(parent()); }
};

} // namespace Sfdk
