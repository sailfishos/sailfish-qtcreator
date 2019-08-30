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

#include "vmconnection_p.h"

#include "virtualboxmanager_p.h"
#include "virtualmachine_p.h"

#include <ssh/sshremoteprocessrunner.h>
#include <utils/qtcassert.h>

#include <QCoreApplication>
#include <QEventLoop>
#include <QTime>
#include <QTimerEvent>

#define DBG qCDebug(vms) << m_vm->name() << QTime::currentTime()

using namespace QSsh;

namespace Sfdk {

using namespace Internal;

namespace {
const int VM_STATE_POLLING_INTERVAL_NORMAL = 10000;
const int VM_STATE_POLLING_INTERVAL_FAST   = 1000;
const int VM_START_TIMEOUT                 = 10000;
const int VM_SOFT_CLOSE_TIMEOUT            = 15000; // via sdk-shutdown command
const int VM_HARD_CLOSE_TIMEOUT            = 15000; // via ACPI poweroff
// Note that SshConnection internally operates with timeouts and has ability to
// recover, but only limited state information is exposed publicly, so these
// numbers cannot really be interpreted as that a new network connection attempt
// will happen every <number> of milliseconds.
const int SSH_TRY_CONNECT_TIMEOUT          = [] {
    bool ok;
    int env = qEnvironmentVariableIntValue("SFDK_VM_CONNECTION_SSH_TRY_CONNECT_TIMEOUT", &ok);
    return ok ? env : 3000;
}();
const int SSH_TRY_CONNECT_INTERVAL_NORMAL  = 1000;
const int SSH_TRY_CONNECT_INTERVAL_SLOW    = 10000;
}

// Most of the obscure handling here is because "shutdown" command newer exits "successfully" :)
class VmConnectionRemoteShutdownProcess : public QObject
{
    Q_OBJECT

public:
    VmConnectionRemoteShutdownProcess(QObject *parent)
        : QObject(parent)
        , m_runner(0)
        , m_connectionErrorOccured(false)
        , m_processStarted(false)
        , m_processClosed(false)
        , m_finished(false)
    {
    }

    void run(const SshConnectionParameters &sshParams)
    {
        delete m_runner, m_runner = new SshRemoteProcessRunner(this);
        m_connectionErrorOccured = false;
        m_connectionErrorString.clear();
        m_processStarted = false;
        m_processClosed = false;
        m_finished = false;
        connect(m_runner, &SshRemoteProcessRunner::processStarted,
                this, &VmConnectionRemoteShutdownProcess::onProcessStarted);
        connect(m_runner, &SshRemoteProcessRunner::processClosed,
                this, &VmConnectionRemoteShutdownProcess::onProcessClosed);
        connect(m_runner, &SshRemoteProcessRunner::connectionError,
                this, &VmConnectionRemoteShutdownProcess::onConnectionError);
        m_runner->run("sdk-shutdown", sshParams);
    }

    // Note: SshRemoteProcessRunner states: "Does not stop remote process, just
    // frees SSH-related process resources."
    void cancel()
    {
        QTC_ASSERT(m_runner, return);
        m_runner->cancel();
    }

    bool isFinished() const { return m_finished; }
    bool isError() const { return isConnectionError() || isProcessError(); }

    bool isConnectionError() const
    {
        return m_finished && !m_processStarted && m_connectionErrorOccured;
    }

    bool isProcessError() const
    {
        return m_finished && m_processClosed &&
            m_runner->processExitStatus() != QProcess::NormalExit;
    }

    QString connectionErrorString() const
    {
        return m_connectionErrorString;
    }

    QString readAllStandardOutput() const
    {
        QTC_ASSERT(m_runner, return QString());
        return QString::fromUtf8(m_runner->readAllStandardOutput());
    }

    QString readAllStandardError() const
    {
        QTC_ASSERT(m_runner, return QString());
        return QString::fromUtf8(m_runner->readAllStandardError());
    }

signals:
    void finished();

private slots:
    void onProcessStarted()
    {
        m_processStarted = true;
    }

    void onProcessClosed()
    {
        m_processClosed = true;
        m_finished = true;
        emit finished();
    }

    void onConnectionError()
    {
        if (!m_processStarted) {
            m_connectionErrorOccured = true;
            m_connectionErrorString = m_runner->lastConnectionErrorString();
        }
        m_finished = true;
        emit finished();
    }

private:
    SshRemoteProcessRunner *m_runner;
    bool m_connectionErrorOccured;
    QString m_connectionErrorString;
    bool m_processStarted;
    bool m_processClosed;
    bool m_finished;
};

VmConnection::VmConnection(VirtualMachine *parent)
    : QObject(parent)
    , m_vm(parent)
    , m_state(VirtualMachine::Disconnected)
    , m_vmState(VmOff)
    , m_vmStartedOutside(false)
    , m_sshState(SshNotConnected)
    , m_vmStmTransition(false) // intentionally do not execute ON_ENTRY during initialization
    , m_sshStmTransition(false) // intentionally do not execute ON_ENTRY during initialization
    , m_lockDownRequested(false)
    , m_lockDownFailed(false)
    , m_connectRequested(false)
    , m_disconnectRequested(false)
    , m_connectLaterRequested(false)
    , m_connectOptions(VirtualMachine::NoConnectOption)
    , m_cachedVmExists(true)
    , m_cachedVmRunning(false)
    , m_cachedVmRunningHeadless(false)
    , m_cachedSshConnected(false)
    , m_cachedSshErrorOccured(false)
    , m_vmWantFastPollState(0)
    , m_pollingVmState(false)
{
    m_vmStateEntryTime.start();

    connect(m_vm, &VirtualMachine::nameChanged, this, [this] () {
        scheduleReset();
        // Do this right now to prevent unexpected behavior
        m_cachedVmExists = true;
        m_cachedVmRunning = false;
        m_cachedVmRunningHeadless = false;
        m_vmStatePollTimer.stop();
    });

    // Notice how SshConnectionParameters::timeout is treated!
    connect(m_vm, &VirtualMachine::sshParametersChanged, this, &VmConnection::scheduleReset);

    connect(m_vm, &VirtualMachine::headlessChanged, this, &VmConnection::scheduleReset);

    connect(m_vm, &VirtualMachine::autoConnectEnabledChanged, this, [this] () {
        vmStmScheduleExec();
        sshStmScheduleExec();
    });
}

VmConnection::~VmConnection()
{
    waitForVmPollStateFinish();
}

VirtualMachine *VmConnection::virtualMachine() const
{
    return m_vm;
}

VirtualMachine::State VmConnection::state() const
{
    return m_state;
}

QString VmConnection::errorString() const
{
    return m_errorString;
}

bool VmConnection::isVirtualMachineOff(bool *runningHeadless, bool *startedOutside) const
{
    if (runningHeadless) {
        if (m_cachedVmRunning)
            *runningHeadless = m_cachedVmRunningHeadless;
        else if (m_vmState == VmStarting) // try to be accurate
            *runningHeadless = m_vm->isHeadless();
        else
            *runningHeadless = false;
    }

    if (startedOutside)
        *startedOutside = m_vmStartedOutside;

    return !m_cachedVmRunning && m_vmState != VmStarting;
}

bool VmConnection::lockDown(bool lockDown)
{
    QTC_ASSERT(m_lockDownRequested != lockDown, return false);

    m_lockDownRequested = lockDown;

    if (m_lockDownRequested) {
        DBG << "Lockdown begin";
        m_connectLaterRequested = false;
        vmPollState(VirtualMachine::Synchronous);
        vmStmScheduleExec();
        sshStmScheduleExec();

        if (isVirtualMachineOff()) {
            return true;
        }

        QEventLoop loop;
        connect(this, &VmConnection::virtualMachineOffChanged,
                &loop, &QEventLoop::quit);
        connect(this, &VmConnection::lockDownFailed,
                &loop, &QEventLoop::quit);
        loop.exec();

        if (m_lockDownFailed) {
            DBG << "Lockdown failed";
            m_lockDownRequested = false;
            m_lockDownFailed = false;
            return false;
        } else {
            return true;
        }
    } else {
        DBG << "Lockdown end";
        vmPollState(VirtualMachine::Asynchronous);
        vmStmScheduleExec();
        sshStmScheduleExec();
        return true;
    }
}

void VmConnection::refresh(VirtualMachine::Synchronization synchronization)
{
    DBG << "Refresh requested; synchronization:" << synchronization;

    vmPollState(synchronization);
}

// Returns false when blocking connection request failed or when did not connect
// immediatelly with non-blocking connection request.
bool VmConnection::connectTo(VirtualMachine::ConnectOptions options)
{
    if (options & VirtualMachine::Block) {
        if (connectTo(options & ~VirtualMachine::Block))
            return true;

        QEventLoop loop;
        connect(this, &VmConnection::stateChanged, &loop, [this, &loop] {
            switch (m_state) {
            case VirtualMachine::Disconnected:
            case VirtualMachine::Error:
                loop.exit(EXIT_FAILURE);
                break;

            case VirtualMachine::Connected:
                loop.exit(EXIT_SUCCESS);
                break;

            default:
                ;
            }
        });

        return loop.exec() == EXIT_SUCCESS;
    }

    DBG << "Connect requested";

    if (!ui()->shouldAsk(Ui::StartVm))
        options &= ~VirtualMachine::AskStartVm;

    // Turning AskStartVm off always overrides
    if ((m_connectOptions & VirtualMachine::AskStartVm) && !(options & VirtualMachine::AskStartVm))
        m_connectOptions &= ~VirtualMachine::AskStartVm;

    vmPollState(VirtualMachine::Asynchronous);
    vmStmScheduleExec();
    sshStmScheduleExec();

    if (m_lockDownRequested) {
        qCWarning(lib) << "VmConnection: connect request for" << m_vm->name() << "ignored: lockdown active";
        return false;
    } else if (m_state == VirtualMachine::Connected) {
        return true;
    } else if (m_connectRequested || m_connectLaterRequested) {
        return false;
    } else if (m_disconnectRequested) {
        ui()->warn(Ui::AlreadyDisconnecting);
        return false;
    }

    if (m_state == VirtualMachine::Error) {
        m_disconnectRequested = true;
        m_connectLaterRequested = true;
        const VirtualMachine::ConnectOptions reconnectOptionsMask =
            VirtualMachine::AskStartVm;
        m_connectOptions = options & ~reconnectOptionsMask;
    } else {
        m_connectRequested = true;
        m_connectOptions = options;
    }

    return false;
}

void VmConnection::disconnectFrom()
{
    DBG << "Disconnect requested";

    if (m_lockDownRequested) {
        return;
    } else if (m_state == VirtualMachine::Disconnected) {
        return;
    } else if (m_disconnectRequested && !m_connectLaterRequested) {
        return;
    } else if (m_connectRequested || m_connectLaterRequested) {
        ui()->warn(Ui::AlreadyConnecting);
        return;
    }

    m_disconnectRequested = true;

    sshStmScheduleExec();
    vmStmScheduleExec();
}

void VmConnection::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == m_vmStartingTimeoutTimer.timerId()) {
        DBG << "VM starting timeout";
        m_vmStartingTimeoutTimer.stop();
        vmStmScheduleExec();
    } else if (event->timerId() == m_vmSoftClosingTimeoutTimer.timerId()) {
        DBG << "VM soft-closing timeout";
        m_vmSoftClosingTimeoutTimer.stop();
        vmStmScheduleExec();
    } else if (event->timerId() == m_vmHardClosingTimeoutTimer.timerId()) {
        DBG << "VM hard-closing timeout";
        m_vmHardClosingTimeoutTimer.stop();
        vmStmScheduleExec();
    } else if (event->timerId() == m_vmStatePollTimer.timerId()) {
        vmPollState(VirtualMachine::Asynchronous);
    } else if (event->timerId() == m_sshTryConnectTimer.timerId()) {
        sshTryConnect();
    } else if (event->timerId() == m_vmStmExecTimer.timerId()) {
        m_vmStmExecTimer.stop();
        vmStmExec();
    } else if (event->timerId() == m_sshStmExecTimer.timerId()) {
        m_sshStmExecTimer.stop();
        sshStmExec();
    } else if (event->timerId() == m_resetTimer.timerId()) {
        m_resetTimer.stop();
        reset();
    }
}

void VmConnection::scheduleReset()
{
    m_resetTimer.start(0, this);
}

void VmConnection::reset()
{
    DBG << "RESET";

    // Just do the best for the possibly updated SSH parameters to get in effect
    if (m_sshState == SshConnectingError || m_sshState == SshConnectionLost)
        delete m_connection; // see sshTryConnect()

    if (m_vm && !m_vmStatePollTimer.isActive()) {
        m_vmStatePollTimer.start(VM_STATE_POLLING_INTERVAL_NORMAL, this);
        vmPollState(VirtualMachine::Asynchronous);
    }

    vmStmScheduleExec();
    sshStmScheduleExec();
}

void VmConnection::updateState()
{
    VirtualMachine::State oldState = m_state;

    switch (m_vmState) {
        case VmOff:
            m_state = VirtualMachine::Disconnected;
            break;

        case VmAskBeforeStarting:
        case VmStarting:
            m_state = VirtualMachine::Starting;
            break;

        case VmStartingError:
            m_state = VirtualMachine::Error;
            m_errorString = tr("Failed to start virtual machine \"%1\"").arg(m_vm->name());
            break;

        case VmRunning:
            switch (m_sshState) {
                case SshNotConnected:
                case SshConnecting:
                    m_state = VirtualMachine::Connecting;
                    break;

                case SshConnectingError:
                    m_state = VirtualMachine::Error;
                    m_errorString = tr("Failed to establish SSH conection with virtual machine "
                            "\"%1\": %2")
                        .arg(m_vm->name())
                        .arg(m_cachedSshErrorString);
                    if (m_vmStateEntryTime.elapsed() > (m_vm->sshParameters().timeout * 1000)) {
                        m_errorString += QString::fromLatin1(" (%1)")
                            .arg(tr("Consider increasing SSH connection timeout in options."));
                    }
                    break;

                case SshConnected:
                    m_state = VirtualMachine::Connected;
                    break;

                case SshDisconnecting:
                case SshDisconnected:
                    m_state = VirtualMachine::Disconnecting;
                    break;

                case SshConnectionLost:
                    m_state = VirtualMachine::Error;
                    m_errorString = tr("SSH conection with virtual machine \"%1\" has been "
                            "lost: %2")
                        .arg(m_vm->name())
                        .arg(m_cachedSshErrorString);
                    break;
            }
            break;

        case VmZombie:
            m_state = VirtualMachine::Disconnected;
            break;

        case VmSoftClosing:
        case VmHardClosing:
            m_state = VirtualMachine::Closing;
            break;
    }

    if (m_state == oldState) {
        return;
    }

    if (m_state != VirtualMachine::Error) {
        m_errorString.clear();
    }

    if (m_state == VirtualMachine::Disconnected && m_connectLaterRequested) {
        m_state = VirtualMachine::Starting; // important
        m_connectLaterRequested = false;
        m_connectRequested = true;
        QTC_CHECK(m_disconnectRequested == false);

        vmPollState(VirtualMachine::Asynchronous);
        vmStmScheduleExec();
        sshStmScheduleExec();
    }

    DBG << "***" << str(oldState) << "-->" << str(m_state);
    if (m_state == VirtualMachine::Error)
        qCWarning(lib) << "VmConnection:" << m_errorString;

    emit stateChanged();
}

void VmConnection::vmStmTransition(VmState toState, const char *event)
{
    DBG << qPrintable(QString::fromLatin1("VM_STATE: %1 --%2--> %3")
            .arg(QLatin1String(str(m_vmState)))
            .arg(QString::fromLatin1(event).replace(QLatin1Char(' '), QLatin1Char('-')))
            .arg(QLatin1String(str(toState))));

    m_vmState = toState;
    m_vmStateEntryTime.restart();
    m_vmStmTransition = true;
}

bool VmConnection::vmStmExiting()
{
    return m_vmStmTransition;
}

bool VmConnection::vmStmEntering()
{
    bool rv = m_vmStmTransition;
    m_vmStmTransition = false;
    return rv;
}

// Avoid invoking directly - use vmStmScheduleExec() to get consecutive events "merged".
void VmConnection::vmStmExec()
{
    bool changed = false;

    while (vmStmStep())
        changed = true;

    if (changed)
        updateState();
}

bool VmConnection::vmStmStep()
{
#define ON_ENTRY if (vmStmEntering())
#define ON_EXIT if (vmStmExiting())

    switch (m_vmState) {
    case VmOff:
        ON_ENTRY {
            m_disconnectRequested = false;
            m_vmStartedOutside = false;
        }

        if (m_cachedVmRunning) {
            m_vmStartedOutside = true;
            vmStmTransition(VmRunning, "started outside");
        } else if (m_lockDownRequested) {
            // noop
        } else if (m_connectRequested) {
            if (m_connectOptions & VirtualMachine::AskStartVm) {
                vmStmTransition(VmAskBeforeStarting, "connect requested&ask before start VM");
            } else {
                vmStmTransition(VmStarting, "connect requested");
            }
        }

        ON_EXIT {
            ;
        }
        break;

    case VmAskBeforeStarting:
        ON_ENTRY {
            ;
        }

        if (m_cachedVmRunning) {
            m_vmStartedOutside = true;
            vmStmTransition(VmRunning, "started outside");
        } else if (m_lockDownRequested) {
            vmStmTransition(VmOff, "lock down requested");
        } else {
            ask(Ui::StartVm, &VmConnection::vmStmScheduleExec,
                    [=] {
                        vmStmTransition(VmStarting, "start VM allowed");
                    },
                    [=] {
                        m_connectRequested = false;
                        m_connectOptions = VirtualMachine::NoConnectOption;
                        vmStmTransition(VmOff, "start VM denied");
                    });
        }

        ON_EXIT {
            ui()->dismissQuestion(Ui::StartVm);
        }
        break;

    case VmStarting:
        ON_ENTRY {
            vmWantFastPollState(true);
            m_vmStartingTimeoutTimer.start(VM_START_TIMEOUT, this);

            VirtualMachinePrivate::get(m_vm)->start(this, [](bool) { /* ignore */ });
        }

        if (m_cachedVmRunning) {
            vmStmTransition(VmRunning, "successfully started");
        } else if (!m_cachedVmExists) {
            ui()->warn(Ui::VmNotRegistered);
            vmStmTransition(VmStartingError, "VM does not exist");
        } else if (!m_vmStartingTimeoutTimer.isActive()) {
            vmStmTransition(VmStartingError, "timeout waiting to start");
        }

        ON_EXIT {
            vmWantFastPollState(false);
            m_vmStartingTimeoutTimer.stop();
        }
        break;

    case VmStartingError:
        ON_ENTRY {
            m_connectRequested = false;
            m_connectOptions = VirtualMachine::NoConnectOption;
        }

        if (m_cachedVmRunning) {
            vmStmTransition(VmRunning, "recovered"); /* spontaneously or with user intervention */
        } else if (m_lockDownRequested) {
            vmStmTransition(VmOff, "lock down requested");
        } else if (m_disconnectRequested) {
            vmStmTransition(VmOff, "disconnect requested");
        }

        ON_EXIT {
            ;
        }
        break;

    case VmRunning:
        ON_ENTRY {
            sshStmScheduleExec();
        }

        if (!m_cachedVmRunning) {
            vmStmTransition(VmOff, "closed outside");
        } else if (m_lockDownRequested) {
            // waiting for ssh connection to disconnect first
            if (m_sshState == SshNotConnected || m_sshState == SshDisconnected) {
                vmStmTransition(VmSoftClosing, "lock down requested");
            }
        } else if (m_disconnectRequested) {
            // waiting for ssh connection to disconnect first
            if (m_sshState == SshNotConnected || m_sshState == SshDisconnected) {
                if (!m_vmStartedOutside && !m_connectLaterRequested) {
                    vmStmTransition(VmSoftClosing, "disconnect requested");
                } else if (m_connectLaterRequested) {
                    ask(Ui::ResetVm, &VmConnection::vmStmScheduleExec,
                            [=] { vmStmTransition(VmSoftClosing, "disconnect&connect later requested+reset allowed"); },
                            [=] { vmStmTransition(VmZombie, "disconnect&connect later requested+reset denied"); });
                } else {
                    ask(Ui::CloseVm, &VmConnection::vmStmScheduleExec,
                            [=] { vmStmTransition(VmSoftClosing, "disconnect requested+close allowed"); },
                            [=] { vmStmTransition(VmZombie, "disconnect requested+close denied"); });
                }
            }
        } else if (m_vmStartedOutside && !m_vm->isAutoConnectEnabled()) {
            if (m_sshState == SshNotConnected || m_sshState == SshDisconnected) {
                vmStmTransition(VmZombie, "started outside+auto connect disabled");
            }
        }

        ON_EXIT {
            ui()->dismissQuestion(Ui::ResetVm);
            ui()->dismissQuestion(Ui::CloseVm);
            sshStmScheduleExec();
        }
        break;

    case VmZombie:
        ON_ENTRY {
            m_disconnectRequested = false;
            QTC_CHECK(!m_lockDownRequested || m_lockDownFailed);
        }

        if (!m_cachedVmRunning) {
            vmStmTransition(VmOff, "closed outside");
        } else if (m_lockDownRequested) {
            if (!m_lockDownFailed) { // prevent endless loop
                vmStmTransition(VmSoftClosing, "lock down requested");
            }
        } else if (m_connectRequested) {
            vmStmTransition(VmRunning, "connect requested");
        }

        ON_EXIT {
            ui()->dismissWarning(Ui::UnableToCloseVm);
        }
        break;

    case VmSoftClosing:
        ON_ENTRY {
            vmWantFastPollState(true);
            m_vmSoftClosingTimeoutTimer.start(VM_SOFT_CLOSE_TIMEOUT, this);

            if (m_connection && m_sshState != SshConnectingError) {
                m_remoteShutdownProcess = new VmConnectionRemoteShutdownProcess(this);
                connect(m_remoteShutdownProcess.data(), &VmConnectionRemoteShutdownProcess::finished,
                        this, &VmConnection::onRemoteShutdownProcessFinished);
                m_remoteShutdownProcess->run(m_connection->connectionParameters());
            } else {
                // see transition on !m_remoteShutdownProcess
            }
        }

        if (!m_cachedVmRunning) {
            vmStmTransition(VmOff, "successfully closed");
        } else if (!m_remoteShutdownProcess) {
            vmStmTransition(VmHardClosing, "no previous successful connection");
        } else if (m_remoteShutdownProcess->isError()) {
            // be quiet - no message box
            if (m_remoteShutdownProcess->isConnectionError()) {
                qCWarning(lib) << "VmConnection: could not connect to the" << m_vm->name()
                    << "virtual machine to soft-close it. Connection error:"
                    << m_remoteShutdownProcess->connectionErrorString();
            } else /* if (m_remoteShutdownProcess->isProcessError()) */ {
                qCWarning(lib) << "VmConnection: failed to soft-close the" << m_vm->name()
                    << "virtual machine. Command output:\n"
                    << "\tSTDOUT [[[" << m_remoteShutdownProcess->readAllStandardOutput() << "]]]\n"
                    << "\tSTDERR [[[" << m_remoteShutdownProcess->readAllStandardError() << "]]]";
            }
            vmStmTransition(VmHardClosing, "failed to soft-close");
        } else if (!m_vmSoftClosingTimeoutTimer.isActive()) {
            // be quiet - no message box
            qCWarning(lib) << "VmConnection: timeout waiting for the" << m_vm->name()
                << "virtual machine to soft-close";
            vmStmTransition(VmHardClosing, "timeout waiting to soft-close");
        }

        ON_EXIT {
            vmWantFastPollState(false);
            m_vmSoftClosingTimeoutTimer.stop();
            delete m_remoteShutdownProcess;
        }
        break;

    case VmHardClosing:
        ON_ENTRY {
            vmWantFastPollState(true);
            m_vmHardClosingTimeoutTimer.start(VM_HARD_CLOSE_TIMEOUT, this);

            VirtualMachinePrivate::get(m_vm)->stop(this, [](bool) { /* ignore */ });
        }

        if (!m_cachedVmRunning) {
            vmStmTransition(VmOff, "successfully closed");
        } else if (!m_vmHardClosingTimeoutTimer.isActive()) {
            qCWarning(lib) << "VmConnection: timeout waiting for the" << m_vm->name()
                << "virtual machine to hard-close.";
            if (m_lockDownRequested) {
                ask(Ui::CancelLockingDown, &VmConnection::vmStmScheduleExec,
                        [=] {
                            m_lockDownFailed = true;
                            emit lockDownFailed();
                            vmStmTransition(VmZombie, "lock down error+retry denied");
                        },
                        [=] {
                            vmStmTransition(VmHardClosing, "lock down error+retry allowed");
                        });
            } else {
                ui()->warn(Ui::UnableToCloseVm); // Keep open until leaving VmZombie
                vmStmTransition(VmZombie, "timeout waiting to hard-close");
            }
        }

        ON_EXIT {
            vmWantFastPollState(false);
            m_vmHardClosingTimeoutTimer.stop();
            ui()->dismissQuestion(Ui::CancelLockingDown);
        }
        break;
    }

    return m_vmStmTransition;

#undef ON_ENTRY
#undef ON_EXIT
}

void VmConnection::sshStmTransition(SshState toState, const char *event)
{
    DBG << qPrintable(QString::fromLatin1("SSH_STATE: %1 --%2--> %3")
            .arg(QLatin1String(str(m_sshState)))
            .arg(QString::fromLatin1(event).replace(QLatin1Char(' '), QLatin1Char('-')))
            .arg(QLatin1String(str(toState))));

    m_sshState = toState;
    m_sshStmTransition = true;
}

bool VmConnection::sshStmExiting()
{
    return m_sshStmTransition;
}

bool VmConnection::sshStmEntering()
{
    bool rv = m_sshStmTransition;
    m_sshStmTransition = false;
    return rv;
}

// Avoid invoking directly - use sshStmScheduleExec() to get consecutive events "merged".
void VmConnection::sshStmExec()
{
    bool changed = false;

    while (sshStmStep())
        changed = true;

    if (changed)
        updateState();
}

bool VmConnection::sshStmStep()
{
#define ON_ENTRY if (sshStmEntering())
#define ON_EXIT if (sshStmExiting())

    switch (m_sshState) {
    case SshNotConnected:
        ON_ENTRY {
            delete m_connection;
            vmStmScheduleExec();
        }

        if (m_lockDownRequested) {
            m_connectRequested = false;
            m_connectOptions = VirtualMachine::NoConnectOption;
        } else if (m_vmState == VmRunning) {
            if (m_connectRequested) {
                sshStmTransition(SshConnecting, "VM running&connect requested");
            } else if (m_vm->isAutoConnectEnabled()) {
                sshStmTransition(SshConnecting, "VM running&auto connect enabled");
            }
        }

        ON_EXIT {
            ;
        }
        break;

    case SshConnecting:
        ON_ENTRY {
            delete m_connection;
            m_cachedSshConnected = false;
            m_cachedSshErrorOccured = false;
            m_cachedSshErrorString.clear();
            createConnection();
            m_connection->connectToHost();
            m_sshTryConnectTimer.start(SSH_TRY_CONNECT_INTERVAL_NORMAL, this);
        }

        if (m_vmState != VmRunning) { // intentionally give precedence over m_cachedSshConnected
            sshStmTransition(SshNotConnected, "VM not running");
        } else if (!m_connectRequested && !m_vm->isAutoConnectEnabled()) {
            sshStmTransition(SshNotConnected, "auto connect disabled while auto connecting");
        } else if (m_cachedSshConnected) {
            sshStmTransition(SshConnected, "successfully connected");
        } else if (m_cachedSshErrorOccured) {
            if (m_vmStartedOutside && !m_connectRequested) {
                sshStmTransition(SshConnectingError, "connecting error+connect not requested");
            } else if (m_vmStateEntryTime.elapsed() < (m_vm->sshParameters().timeout * 1000)) {
                ; // Do not report possibly recoverable boot-time failure
            } else {
                ask(Ui::CancelConnecting, &VmConnection::sshStmScheduleExec,
                        [=] { sshStmTransition(SshConnectingError, "connecting error+retry denied"); },
                        [=] { sshStmTransition(SshConnecting, "connecting error+retry allowed"); });
            }
        }

        ON_EXIT {
            m_sshTryConnectTimer.stop();
            ui()->dismissQuestion(Ui::CancelConnecting);
        }
        break;

    case SshConnectingError:
        ON_ENTRY {
            m_connectRequested = false;
            m_connectOptions = VirtualMachine::NoConnectOption;
            m_sshTryConnectTimer.start(SSH_TRY_CONNECT_INTERVAL_SLOW, this);
        }

        if (m_vmState != VmRunning) { // intentionally give precedence over m_cachedSshConnected
            sshStmTransition(SshNotConnected, "VM not running");
        } else if (m_cachedSshConnected) {
            sshStmTransition(SshConnected, "recovered");
        } else if (m_lockDownRequested) {
            sshStmTransition(SshNotConnected, "lock down requested");
        } else if (m_disconnectRequested) {
            sshStmTransition(SshDisconnected, "disconnect requested");
        }

        ON_EXIT {
            m_sshTryConnectTimer.stop();
        }
        break;

    case SshConnected:
        ON_ENTRY {
            m_connectRequested = false;
            m_connectOptions = VirtualMachine::NoConnectOption;
        }

        if (m_vmState != VmRunning) {
            sshStmTransition(SshNotConnected, "VM not running");
        } else if (!m_cachedSshConnected) {
            sshStmTransition(SshConnectionLost, "connection lost");
        } else if (m_lockDownRequested) {
            sshStmTransition(SshDisconnecting, "lock down requested");
        } else if (m_disconnectRequested) {
            sshStmTransition(SshDisconnecting, "disconnect requested");
        }

        ON_EXIT {
            ;
        }
        break;

    case SshDisconnecting:
        ON_ENTRY {
            m_connection->disconnectFromHost();
        }

        if (!m_cachedSshConnected) {
            sshStmTransition(SshDisconnected, "successfully disconnected");
        }

        ON_EXIT {
            ;
        }
        break;

    case SshDisconnected:
        ON_ENTRY {
            delete m_connection;
            vmStmScheduleExec();
        }

        if (m_vmState != VmRunning) {
            sshStmTransition(SshNotConnected, "VM not running");
        } else if (m_connectRequested) {
            sshStmTransition(SshConnecting, "connect requested");
        }

        ON_EXIT {
            ;
        }
        break;

    case SshConnectionLost:
        ON_ENTRY {
            vmWantFastPollState(true);
            m_sshTryConnectTimer.start(SSH_TRY_CONNECT_INTERVAL_NORMAL, this);
        }

        if (m_vmState != VmRunning) { // intentionally give precedence over m_cachedSshConnected
            sshStmTransition(SshNotConnected, "VM not running");
        } else if (m_cachedSshConnected) {
            sshStmTransition(SshConnected, "recovered");
        } else if (m_lockDownRequested) {
            sshStmTransition(SshDisconnected, "lock down requested");
        } else if (m_disconnectRequested) {
            sshStmTransition(SshDisconnected, "disconnect requested");
        }

        ON_EXIT {
            vmWantFastPollState(false);
            m_sshTryConnectTimer.stop();
        }
        break;
    }

    return m_sshStmTransition;

#undef ON_ENTRY
#undef ON_EXIT
}

void VmConnection::createConnection()
{
    QTC_CHECK(m_connection == 0);

    SshConnectionParameters params(m_vm->sshParameters());
    params.timeout = SSH_TRY_CONNECT_TIMEOUT / 1000;
    m_connection = new SshConnection(params, this);
    connect(m_connection.data(), &SshConnection::connected,
            this, &VmConnection::onSshConnected);
    connect(m_connection.data(), &SshConnection::disconnected,
            this, &VmConnection::onSshDisconnected);
    connect(m_connection.data(), &SshConnection::errorOccurred,
            this, &VmConnection::onSshErrorOccured);
}

void VmConnection::vmWantFastPollState(bool want)
{
    if (want) {
        // max one for VM state machine, one for SSH state machine
        QTC_CHECK(m_vmWantFastPollState < 2);
        if (++m_vmWantFastPollState == 1) {
            DBG << "Start fast VM state polling";
            m_vmStatePollTimer.start(VM_STATE_POLLING_INTERVAL_FAST, this);
        }
    } else {
        QTC_CHECK(m_vmWantFastPollState > 0);
        if (--m_vmWantFastPollState == 0) {
            DBG << "Stop fast VM state polling";
            m_vmStatePollTimer.start(VM_STATE_POLLING_INTERVAL_NORMAL, this);
        }
    }
}

void VmConnection::vmPollState(VirtualMachine::Synchronization synchronization)
{
    if (m_pollingVmState) {
        if (synchronization == VirtualMachine::Synchronous) {
            DBG << "Already polling - waiting";
            waitForVmPollStateFinish();
        } else {
            DBG << "Already polling";
        }
        return;
    }

    m_pollingVmState = true;

    QEventLoop *loop = 0;
    if (synchronization == VirtualMachine::Synchronous)
        loop = new QEventLoop(this);

    auto handler = [this, loop](VirtualMachinePrivate::BasicState state, bool success) {
        if (!success) {
            m_pollingVmState = false;
            if (loop)
                loop->quit();
            return;
        }

        const bool vmExists = state & VirtualMachinePrivate::Existing;
        const bool vmRunning = state & VirtualMachinePrivate::Running;
        const bool vmHeadless = state & VirtualMachinePrivate::Headless;

        bool changed = false;

        if (vmRunning != m_cachedVmRunning) {
            DBG << "VM running:" << m_cachedVmRunning << "-->" << vmRunning;
            m_cachedVmRunning = vmRunning;
            m_cachedVmRunningHeadless = vmHeadless;
            emit virtualMachineOffChanged(!m_cachedVmRunning);
            changed = true;
        }

        if (vmExists != m_cachedVmExists) {
            DBG << "VM exists:" << m_cachedVmExists << "-->" << vmExists;
            m_cachedVmExists = vmExists;
            changed = true;
        }

        if (changed)
            vmStmScheduleExec();

        m_pollingVmState = false;

        if (loop)
            loop->quit();
    };

    VirtualMachinePrivate::get(m_vm)->probe(this, handler);

    if (synchronization == VirtualMachine::Synchronous) {
        loop->exec();
        delete loop, loop = 0;
    }
}

void VmConnection::waitForVmPollStateFinish()
{
    while (m_pollingVmState)
        QCoreApplication::processEvents();
}

void VmConnection::sshTryConnect()
{
    if (!m_connection || (m_connection->state() == SshConnection::Unconnected &&
                /* Important: retry only after an SSH connection error is reported to us! Otherwise
                 * we would end trying-again endlessly, suppressing any SSH error. */
                m_cachedSshErrorOccured && m_cachedSshErrorOrigin == m_connection)) {
        if (m_connection)
            DBG << "SSH try connect - previous error:" << m_cachedSshErrorString;
        else
            DBG << "SSH try connect - no active connection";
        delete m_connection;
        createConnection();
        m_connection->connectToHost();
    }
}

const char *VmConnection::str(VirtualMachine::State state)
{
    static const char *strings[] = {
        "Disconnected",
        "Starting",
        "Connecting",
        "Error",
        "Connected",
        "Disconnecting",
        "Closing",
    };

    return strings[state];
}

const char *VmConnection::str(VmState vmState)
{
    static const char *strings[] = {
        "VmOff",
        "VmAskBeforeStarting",
        "VmStarting",
        "VmStartingError",
        "VmRunning",
        "VmSoftClosing",
        "VmHardClosing",
        "VmZombie",
    };

    return strings[vmState];
}

const char *VmConnection::str(SshState sshState)
{
    static const char *strings[] = {
        "SshNotConnected",
        "SshConnecting",
        "SshConnectingError",
        "SshConnected",
        "SshDisconnecting",
        "SshDisconnected",
        "SshConnectionLost",
    };

    return strings[sshState];
}

void VmConnection::vmStmScheduleExec()
{
    m_vmStmExecTimer.start(0, this);
}

void VmConnection::sshStmScheduleExec()
{
    m_sshStmExecTimer.start(0, this);
}

void VmConnection::onSshConnected()
{
    DBG << "SSH connected";
    m_cachedSshConnected = true;
    m_cachedSshErrorOccured = false;
    m_cachedSshErrorString.clear();
    sshStmScheduleExec();
}

void VmConnection::onSshDisconnected()
{
    DBG << "SSH disconnected";
    m_cachedSshConnected = false;
    vmPollState(VirtualMachine::Asynchronous);
    sshStmScheduleExec();
}

void VmConnection::onSshErrorOccured()
{
    DBG << "SSH error:" << m_connection->errorString();
    m_cachedSshErrorOccured = true;
    m_cachedSshErrorString = m_connection->errorString();
    m_cachedSshErrorOrigin = m_connection;
    vmPollState(VirtualMachine::Asynchronous);
    sshStmScheduleExec();
}

void VmConnection::onRemoteShutdownProcessFinished()
{
    DBG << "Remote shutdown process finished. Succeeded:" << !m_remoteShutdownProcess->isError();
    vmStmScheduleExec();
}

VirtualMachine::ConnectionUi *VmConnection::ui() const
{
    return VirtualMachinePrivate::get(m_vm)->connectionUi();
}

void VmConnection::ask(Ui::Question which,
        void (VmConnection::*onStatusChanged)(),
        std::function<void()> ifYes, std::function<void()> ifNo)
{
    switch (ui()->status(which)) {
    case Ui::NotAsked:
        ui()->ask(which, [=] { (this->*onStatusChanged)(); });
        break;
    case Ui::Asked:
        break;
    case Ui::Yes:
        ifYes();
        break;
    case Ui::No:
        ifNo();
        break;
    }
}

} // Sfdk

#include "vmconnection.moc"
