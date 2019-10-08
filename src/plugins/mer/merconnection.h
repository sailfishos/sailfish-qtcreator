/****************************************************************************
**
** Copyright (C) 2012-2019 Jolla Ltd.
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
#include <QTime>
#include <QtGlobal>

#include <functional>

QT_BEGIN_NAMESPACE
class QMessageBox;
class QProgressDialog;
QT_END_NAMESPACE

namespace Mer {

class MerConnectionRemoteShutdownProcess;

class Q_DECL_EXPORT MerConnection : public QObject
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

    bool isVirtualMachineOff(bool *runningHeadless = 0, bool *startedOutside = 0) const;
    bool lockDown(bool lockDown);

    static QStringList usedVirtualMachines();

    template<typename ConcreteUi>
    static void registerUi()
    {
        Q_ASSERT(!s_uiCreator);
        s_uiCreator = [](MerConnection *parent) { return new ConcreteUi(parent); };
    }

public slots:
    void refresh(Mer::MerConnection::Synchronization synchronization = Asynchronous);
    bool connectTo(Mer::MerConnection::ConnectOptions options = NoConnectOption);
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
    using UiCreator = std::function<Ui *(MerConnection *)>;
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
    QPointer<MerConnectionRemoteShutdownProcess> m_remoteShutdownProcess;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(MerConnection::ConnectOptions)

class MerConnection::Ui : public QObject
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

    using OnStatusChanged = void (MerConnection::*)();

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
    MerConnection *connection() const { return static_cast<MerConnection *>(parent()); }
};

#ifdef MER_LIBRARY
class MerConnectionWidgetUi : public MerConnection::Ui
{
    Q_OBJECT

public:
    using MerConnection::Ui::Ui;

    void warn(Warning which) override;
    void dismissWarning(Warning which) override;

    bool shouldAsk(Question which) const override;
    void ask(Question which, OnStatusChanged onStatusChanged) override;
    void dismissQuestion(Question which) override;
    QuestionStatus status(Question which) const override;

private:
    QMessageBox *openWarningBox(const QString &title, const QString &text);
    QMessageBox *openQuestionBox(OnStatusChanged onStatusChanged,
            const QString &title, const QString &text,
            const QString &informativeText = QString(),
            std::function<void()> setDoNotAskAgain = nullptr);
    QProgressDialog *openProgressDialog(OnStatusChanged onStatusChanged,
            const QString &title, const QString &text);
    template<class Dialog>
    void deleteDialog(QPointer<Dialog> &dialog);
    QuestionStatus status(QMessageBox *box) const;
    QuestionStatus status(QProgressDialog *dialog) const;

private:
    QPointer<QMessageBox> m_unableToCloseVmWarningBox;
    QPointer<QMessageBox> m_startVmQuestionBox;
    QPointer<QMessageBox> m_resetVmQuestionBox;
    QPointer<QMessageBox> m_closeVmQuestionBox;
    QPointer<QProgressDialog> m_connectingProgressDialog;
    QPointer<QProgressDialog> m_lockingDownProgressDialog;
};
#endif // MER_LIBRARY

}
#endif
