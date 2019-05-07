/****************************************************************************
**
** Copyright (C) 2012 - 2016 Jolla Ltd.
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
#include "merlogging.h"
#include "mersettings.h"
#include "mervirtualboxmanager.h"

#include <coreplugin/icore.h>
#include <ssh/sshremoteprocessrunner.h>
#include <utils/qtcassert.h>
#include <utils/checkablemessagebox.h>

#include <QCheckBox>
#include <QCoreApplication>
#include <QEventLoop>
#include <QMessageBox>
#include <QProgressDialog>
#include <QTime>
#include <QTimer>
#include <QTimerEvent>

#define DBG qCDebug(Log::vms) << m_vmName << QTime::currentTime()

using namespace Core;
using namespace QSsh;
using Utils::CheckableMessageBox;

namespace Mer {

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
    int env = qEnvironmentVariableIntValue("MER_SSH_TRY_CONNECT_TIMEOUT", &ok);
    return ok ? env : 3000;
}();
const int SSH_TRY_CONNECT_INTERVAL_NORMAL  = 1000;
const int SSH_TRY_CONNECT_INTERVAL_SLOW    = 10000;
const int DISMISS_MESSAGE_BOX_DELAY        = 2000;
}

// Most of the obscure handling here is because "shutdown" command newer exits "successfully" :)
class MerConnectionRemoteShutdownProcess : public QObject
{
    Q_OBJECT

public:
    MerConnectionRemoteShutdownProcess(QObject *parent)
        : QObject(parent)
        , m_runner(0)
        , m_connectionError(SshNoError)
        , m_processStarted(false)
        , m_processClosed(false)
        , m_exitStatus(SshRemoteProcess::FailedToStart)
        , m_finished(false)
    {
    }

    void run(const SshConnectionParameters &sshParams)
    {
        delete m_runner, m_runner = new SshRemoteProcessRunner(this);
        m_connectionError = SshNoError;
        m_connectionErrorString.clear();
        m_processStarted = false;
        m_processClosed = false;
        m_exitStatus = SshRemoteProcess::FailedToStart;
        m_finished = false;
        connect(m_runner, &SshRemoteProcessRunner::processStarted,
                this, &MerConnectionRemoteShutdownProcess::onProcessStarted);
        connect(m_runner, &SshRemoteProcessRunner::processClosed,
                this, &MerConnectionRemoteShutdownProcess::onProcessClosed);
        connect(m_runner, &SshRemoteProcessRunner::connectionError,
                this, &MerConnectionRemoteShutdownProcess::onConnectionError);
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
        return m_finished && !m_processStarted &&
            m_connectionError != SshNoError;
    }

    bool isProcessError() const
    {
        return m_finished && m_processClosed &&
            m_exitStatus != SshRemoteProcess::NormalExit;
    }

    SshError connectionError() const
    {
        return m_connectionError;
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

    void onProcessClosed(int exitStatus)
    {
        m_processClosed = true;
        m_exitStatus = exitStatus;
        m_finished = true;
        emit finished();
    }

    void onConnectionError()
    {
        if (!m_processStarted) {
            m_connectionError = m_runner->lastConnectionError();
            m_connectionErrorString = m_runner->lastConnectionErrorString();
        }
        m_finished = true;
        emit finished();
    }

private:
    SshRemoteProcessRunner *m_runner;
    SshError m_connectionError;
    QString m_connectionErrorString;
    bool m_processStarted;
    bool m_processClosed;
    int m_exitStatus;
    bool m_finished;
};

MerConnection::UiCreator MerConnection::s_uiCreator;
QMap<QString, int> MerConnection::s_usedVmNames;

MerConnection::MerConnection(QObject *parent)
    : QObject(parent)
    , m_headless(false)
    , m_state(Disconnected)
    , m_vmState(VmOff)
    , m_vmStartedOutside(false)
    , m_sshState(SshNotConnected)
    , m_vmStmTransition(false) // intentionally do not execute ON_ENTRY during initialization
    , m_sshStmTransition(false) // intentionally do not execute ON_ENTRY during initialization
    , m_lockDownRequested(false)
    , m_lockDownFailed(false)
    , m_autoConnectEnabled(true)
    , m_connectRequested(false)
    , m_disconnectRequested(false)
    , m_connectLaterRequested(false)
    , m_connectOptions(NoConnectOption)
    , m_cachedVmExists(true)
    , m_cachedVmRunning(false)
    , m_cachedSshConnected(false)
    , m_cachedSshError(SshNoError)
    , m_vmWantFastPollState(0)
    , m_pollingVmState(false)
    , m_ui(s_uiCreator(this))
{
    m_vmStateEntryTime.start();
    m_ui->setParent(this);
}

MerConnection::~MerConnection()
{
    waitForVmPollStateFinish();

    if (!m_vmName.isEmpty()) {
        if (--s_usedVmNames[m_vmName] == 0)
            s_usedVmNames.remove(m_vmName);
    }
}

void MerConnection::setVirtualMachine(const QString &virtualMachine)
{
    if (m_vmName == virtualMachine)
        return;

    if (!m_vmName.isEmpty()) {
        if (--s_usedVmNames[m_vmName] == 0)
            s_usedVmNames.remove(m_vmName);
    }

    m_vmName = virtualMachine;
    scheduleReset();

    // Do this right now to prevent unexpected behavior
    m_cachedVmExists = true;
    m_cachedVmRunning = false;
    m_vmStatePollTimer.stop();

    if (!m_vmName.isEmpty()) {
        if (++s_usedVmNames[m_vmName] != 1)
            qWarning() << "MerConnection: Another instance for VM" << m_vmName << "already exists";
    }

    emit virtualMachineChanged();
}

void MerConnection::setSshParameters(const SshConnectionParameters &sshParameters)
{
    if (m_params == sshParameters)
        return;

    // Notice how m_params.timeout is treated!
    m_params = sshParameters;
    scheduleReset();
}

void MerConnection::setHeadless(bool headless)
{
    if (m_headless == headless)
        return;

    m_headless = headless;
    scheduleReset();
}

SshConnectionParameters MerConnection::sshParameters() const
{
    return m_params;
}

bool MerConnection::isHeadless() const
{
    return m_headless;
}

QString MerConnection::virtualMachine() const
{
    return m_vmName;
}

MerConnection::State MerConnection::state() const
{
    return m_state;
}

QString MerConnection::errorString() const
{
    return m_errorString;
}

bool MerConnection::isAutoConnectEnabled() const
{
    return m_autoConnectEnabled;
}

void MerConnection::setAutoConnectEnabled(bool autoConnectEnabled)
{
    QTC_ASSERT(m_autoConnectEnabled != autoConnectEnabled, return);

    m_autoConnectEnabled = autoConnectEnabled;

    vmStmScheduleExec();
    sshStmScheduleExec();
}

bool MerConnection::isVirtualMachineOff(bool *runningHeadless, bool *startedOutside) const
{
    if (runningHeadless) {
        if (m_cachedVmRunning)
            *runningHeadless = MerVirtualBoxManager::fetchVirtualMachineInfo(m_vmName).headless;
        else if (m_vmState == VmStarting) // try to be accurate
            *runningHeadless = m_headless;
        else
            *runningHeadless = false;
    }

    if (startedOutside)
        *startedOutside = m_vmStartedOutside;

    return !m_cachedVmRunning && m_vmState != VmStarting;
}

bool MerConnection::lockDown(bool lockDown)
{
    QTC_ASSERT(m_lockDownRequested != lockDown, return false);

    m_lockDownRequested = lockDown;

    if (m_lockDownRequested) {
        DBG << "Lockdown begin";
        m_connectLaterRequested = false;
        vmPollState(Synchronous);
        vmStmScheduleExec();
        sshStmScheduleExec();

        if (isVirtualMachineOff()) {
            return true;
        }

        QEventLoop loop;
        connect(this, &MerConnection::virtualMachineOffChanged,
                &loop, &QEventLoop::quit);
        connect(this, &MerConnection::lockDownFailed,
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
        vmPollState(Asynchronous);
        vmStmScheduleExec();
        sshStmScheduleExec();
        return true;
    }
}

// Rationale: Consider the use case of adding a new SDK/emulator. User should
// be presented with the list of all _unused_ VMs. It is not enough to simply
// collect VMs associated with all MerSdkManager::sdks and
// DeviceManager::devices of type MerEmulatorDevice - until the button Apply/OK
// is clicked, some instances may exist not reachable this way.
QStringList MerConnection::usedVirtualMachines()
{
    return s_usedVmNames.keys();
}

void MerConnection::refresh(Synchronization synchronization)
{
    DBG << "Refresh requested; synchronization:" << synchronization;

    vmPollState(synchronization);
}

// Returns false when blocking connection request failed or when did not connect
// immediatelly with non-blocking connection request.
bool MerConnection::connectTo(ConnectOptions options)
{
    if (options & Block) {
        if (connectTo(options & ~Block))
            return true;

        QEventLoop loop;
        connect(this, &MerConnection::stateChanged, &loop, [this, &loop] {
            switch (m_state) {
            case MerConnection::Disconnected:
            case MerConnection::Error:
                loop.exit(EXIT_FAILURE);
                break;

            case MerConnection::Connected:
                loop.exit(EXIT_SUCCESS);
                break;

            default:
                ;
            }
        });

        return loop.exec() == EXIT_SUCCESS;
    }

    DBG << "Connect requested";

    if (!m_ui->shouldAsk(Ui::StartVm))
        options &= ~AskStartVm;

    // Turning AskStartVm off always overrides
    if ((m_connectOptions & AskStartVm) && !(options & AskStartVm))
        m_connectOptions &= ~AskStartVm;

    vmPollState(Asynchronous);
    vmStmScheduleExec();
    sshStmScheduleExec();

    if (m_lockDownRequested) {
        qWarning() << "MerConnection: connect request for" << m_vmName << "ignored: lockdown active";
        return false;
    } else if (m_state == Connected) {
        return true;
    } else if (m_connectRequested || m_connectLaterRequested) {
        return false;
    } else if (m_disconnectRequested) {
        m_ui->warn(Ui::AlreadyDisconnecting);
        return false;
    }

    if (m_state == Error) {
        m_disconnectRequested = true;
        m_connectLaterRequested = true;
        const ConnectOptions reconnectOptionsMask =
            AskStartVm;
        m_connectOptions = options & ~reconnectOptionsMask;
    } else {
        m_connectRequested = true;
        m_connectOptions = options;
    }

    return false;
}

void MerConnection::disconnectFrom()
{
    DBG << "Disconnect requested";

    if (m_lockDownRequested) {
        return;
    } else if (m_state == Disconnected) {
        return;
    } else if (m_disconnectRequested && !m_connectLaterRequested) {
        return;
    } else if (m_connectRequested || m_connectLaterRequested) {
        m_ui->warn(Ui::AlreadyConnecting);
        return;
    }

    m_disconnectRequested = true;

    sshStmScheduleExec();
    vmStmScheduleExec();
}

void MerConnection::timerEvent(QTimerEvent *event)
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
        vmPollState(Asynchronous);
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

void MerConnection::scheduleReset()
{
    m_resetTimer.start(0, this);
}

void MerConnection::reset()
{
    DBG << "RESET";

    // Just do the best for the possibly updated SSH parameters to get in effect
    if (m_sshState == SshConnectingError || m_sshState == SshConnectionLost)
        delete m_connection; // see sshTryConnect()

    if (!m_vmName.isEmpty() && !m_vmStatePollTimer.isActive()) {
        m_vmStatePollTimer.start(VM_STATE_POLLING_INTERVAL_NORMAL, this);
        vmPollState(Asynchronous);
    }

    vmStmScheduleExec();
    sshStmScheduleExec();
}

void MerConnection::updateState()
{
    State oldState = m_state;

    switch (m_vmState) {
        case VmOff:
            m_state = Disconnected;
            break;

        case VmAskBeforeStarting:
        case VmStarting:
            m_state = StartingVm;
            break;

        case VmStartingError:
            m_state = Error;
            m_errorString = tr("Failed to start virtual machine \"%1\"").arg(m_vmName);
            break;

        case VmRunning:
            switch (m_sshState) {
                case SshNotConnected:
                case SshConnecting:
                    m_state = Connecting;
                    break;

                case SshConnectingError:
                    m_state = Error;
                    m_errorString = tr("Failed to establish SSH conection with virtual machine "
                            "\"%1\": %2 %3")
                        .arg(m_vmName)
                        .arg((int)m_cachedSshError)
                        .arg(m_cachedSshErrorString);
                    if (isRecoverable(m_cachedSshError)) {
                        m_errorString += QString::fromLatin1(" (%1)")
                            .arg(tr("Consider increasing SSH connection timeout in options."));
                    }
                    break;

                case SshConnected:
                    m_state = Connected;
                    break;

                case SshDisconnecting:
                case SshDisconnected:
                    m_state = Disconnecting;
                    break;

                case SshConnectionLost:
                    m_state = Error;
                    m_errorString = tr("SSH conection with virtual machine \"%1\" has been "
                            "lost: %2 %3")
                        .arg(m_vmName)
                        .arg((int)m_cachedSshError)
                        .arg(m_cachedSshErrorString);
                    break;
            }
            break;

        case VmZombie:
            m_state = Disconnected;
            break;

        case VmSoftClosing:
        case VmHardClosing:
            m_state = ClosingVm;
            break;
    }

    if (m_state == oldState) {
        return;
    }

    if (m_state != Error) {
        m_errorString.clear();
    }

    if (m_state == Disconnected && m_connectLaterRequested) {
        m_state = StartingVm; // important
        m_connectLaterRequested = false;
        m_connectRequested = true;
        QTC_CHECK(m_disconnectRequested == false);

        vmPollState(Asynchronous);
        vmStmScheduleExec();
        sshStmScheduleExec();
    }

    DBG << "***" << str(oldState) << "-->" << str(m_state);
    if (m_state == Error)
        qWarning() << "MerConnection:" << m_errorString;

    emit stateChanged();
}

void MerConnection::vmStmTransition(VmState toState, const char *event)
{
    DBG << qPrintable(QString::fromLatin1("VM_STATE: %1 --%2--> %3")
            .arg(QLatin1String(str(m_vmState)))
            .arg(QString::fromLatin1(event).replace(QLatin1Char(' '), QLatin1Char('-')))
            .arg(QLatin1String(str(toState))));

    m_vmState = toState;
    m_vmStateEntryTime.restart();
    m_vmStmTransition = true;
}

bool MerConnection::vmStmExiting()
{
    return m_vmStmTransition;
}

bool MerConnection::vmStmEntering()
{
    bool rv = m_vmStmTransition;
    m_vmStmTransition = false;
    return rv;
}

// Avoid invoking directly - use vmStmScheduleExec() to get consecutive events "merged".
void MerConnection::vmStmExec()
{
    bool changed = false;

    while (vmStmStep())
        changed = true;

    if (changed)
        updateState();
}

bool MerConnection::vmStmStep()
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
            if (m_connectOptions & AskStartVm) {
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
            m_ui->ask(Ui::StartVm, &MerConnection::vmStmScheduleExec,
                    [=] {
                        vmStmTransition(VmStarting, "start VM allowed");
                    },
                    [=] {
                        m_connectRequested = false;
                        m_connectOptions = NoConnectOption;
                        vmStmTransition(VmOff, "start VM denied");
                    });
        }

        ON_EXIT {
            m_ui->dismissQuestion(Ui::StartVm);
        }
        break;

    case VmStarting:
        ON_ENTRY {
            vmWantFastPollState(true);
            m_vmStartingTimeoutTimer.start(VM_START_TIMEOUT, this);

            MerVirtualBoxManager::startVirtualMachine(m_vmName, m_headless);
        }

        if (m_cachedVmRunning) {
            vmStmTransition(VmRunning, "successfully started");
        } else if (!m_cachedVmExists) {
            m_ui->warn(Ui::VmNotRegistered);
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
            m_connectOptions = NoConnectOption;
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
                    m_ui->ask(Ui::ResetVm, &MerConnection::vmStmScheduleExec,
                            [=] { vmStmTransition(VmSoftClosing, "disconnect&connect later requested+reset allowed"); },
                            [=] { vmStmTransition(VmZombie, "disconnect&connect later requested+reset denied"); });
                } else {
                    m_ui->ask(Ui::CloseVm, &MerConnection::vmStmScheduleExec,
                            [=] { vmStmTransition(VmSoftClosing, "disconnect requested+close allowed"); },
                            [=] { vmStmTransition(VmZombie, "disconnect requested+close denied"); });
                }
            }
        } else if (m_vmStartedOutside && !m_autoConnectEnabled) {
            if (m_sshState == SshNotConnected || m_sshState == SshDisconnected) {
                vmStmTransition(VmZombie, "started outside+auto connect disabled");
            }
        }

        ON_EXIT {
            m_ui->dismissQuestion(Ui::ResetVm);
            m_ui->dismissQuestion(Ui::CloseVm);
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
            m_ui->dismissWarning(Ui::UnableToCloseVm);
        }
        break;

    case VmSoftClosing:
        ON_ENTRY {
            vmWantFastPollState(true);
            m_vmSoftClosingTimeoutTimer.start(VM_SOFT_CLOSE_TIMEOUT, this);

            if (m_connection && m_sshState != SshConnectingError) {
                m_remoteShutdownProcess = new MerConnectionRemoteShutdownProcess(this);
                connect(m_remoteShutdownProcess.data(), &MerConnectionRemoteShutdownProcess::finished,
                        this, &MerConnection::onRemoteShutdownProcessFinished);
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
                qWarning() << "MerConnection: could not connect to the" << m_vmName
                    << "virtual machine to soft-close it. Connection error:"
                    << m_remoteShutdownProcess->connectionError()
                    << m_remoteShutdownProcess->connectionErrorString();
            } else /* if (m_remoteShutdownProcess->isProcessError()) */ {
                qWarning() << "MerConnection: failed to soft-close the" << m_vmName
                    << "virtual machine. Command output:\n"
                    << "\tSTDOUT [[[" << m_remoteShutdownProcess->readAllStandardOutput() << "]]]\n"
                    << "\tSTDERR [[[" << m_remoteShutdownProcess->readAllStandardError() << "]]]";
            }
            vmStmTransition(VmHardClosing, "failed to soft-close");
        } else if (!m_vmSoftClosingTimeoutTimer.isActive()) {
            // be quiet - no message box
            qWarning() << "MerConnection: timeout waiting for the" << m_vmName
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

            MerVirtualBoxManager::shutVirtualMachine(m_vmName);
        }

        if (!m_cachedVmRunning) {
            vmStmTransition(VmOff, "successfully closed");
        } else if (!m_vmHardClosingTimeoutTimer.isActive()) {
            qWarning() << "MerConnection: timeout waiting for the" << m_vmName
                << "virtual machine to hard-close.";
            if (m_lockDownRequested) {
                m_ui->ask(Ui::CancelLockingDown, &MerConnection::vmStmScheduleExec,
                        [=] {
                            m_lockDownFailed = true;
                            emit lockDownFailed();
                            vmStmTransition(VmZombie, "lock down error+retry denied");
                        },
                        [=] {
                            vmStmTransition(VmHardClosing, "lock down error+retry allowed");
                        });
            } else {
                m_ui->warn(Ui::UnableToCloseVm); // Keep open until leaving VmZombie
                vmStmTransition(VmZombie, "timeout waiting to hard-close");
            }
        }

        ON_EXIT {
            vmWantFastPollState(false);
            m_vmHardClosingTimeoutTimer.stop();
            m_ui->dismissQuestion(Ui::CancelLockingDown);
        }
        break;
    }

    return m_vmStmTransition;

#undef ON_ENTRY
#undef ON_EXIT
}

void MerConnection::sshStmTransition(SshState toState, const char *event)
{
    DBG << qPrintable(QString::fromLatin1("SSH_STATE: %1 --%2--> %3")
            .arg(QLatin1String(str(m_sshState)))
            .arg(QString::fromLatin1(event).replace(QLatin1Char(' '), QLatin1Char('-')))
            .arg(QLatin1String(str(toState))));

    m_sshState = toState;
    m_sshStmTransition = true;
}

bool MerConnection::sshStmExiting()
{
    return m_sshStmTransition;
}

bool MerConnection::sshStmEntering()
{
    bool rv = m_sshStmTransition;
    m_sshStmTransition = false;
    return rv;
}

// Avoid invoking directly - use sshStmScheduleExec() to get consecutive events "merged".
void MerConnection::sshStmExec()
{
    bool changed = false;

    while (sshStmStep())
        changed = true;

    if (changed)
        updateState();
}

bool MerConnection::sshStmStep()
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
            m_connectOptions = NoConnectOption;
        } else if (m_vmState == VmRunning) {
            if (m_connectRequested) {
                sshStmTransition(SshConnecting, "VM running&connect requested");
            } else if (m_autoConnectEnabled) {
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
            m_cachedSshError = SshNoError;
            m_cachedSshErrorString.clear();
            createConnection();
            m_connection->connectToHost();
            m_sshTryConnectTimer.start(SSH_TRY_CONNECT_INTERVAL_NORMAL, this);
        }

        if (m_vmState != VmRunning) { // intentionally give precedence over m_cachedSshConnected
            sshStmTransition(SshNotConnected, "VM not running");
        } else if (!m_connectRequested && !m_autoConnectEnabled) {
            sshStmTransition(SshNotConnected, "auto connect disabled while auto connecting");
        } else if (m_cachedSshConnected) {
            sshStmTransition(SshConnected, "successfully connected");
        } else if (m_cachedSshError != SshNoError) {
            if (m_vmStartedOutside && !m_connectRequested) {
                sshStmTransition(SshConnectingError, "connecting error+connect not requested");
            } else if (m_vmStateEntryTime.elapsed() < (m_params.timeout * 1000) &&
                       isRecoverable(m_cachedSshError)) {
                ; // Do not report possibly recoverable boot-time failure
            } else {
                m_ui->ask(Ui::CancelConnecting, &MerConnection::sshStmScheduleExec,
                        [=] { sshStmTransition(SshConnectingError, "connecting error+retry denied"); },
                        [=] { sshStmTransition(SshConnecting, "connecting error+retry allowed"); });
            }
        }

        ON_EXIT {
            m_sshTryConnectTimer.stop();
            m_ui->dismissQuestion(Ui::CancelConnecting);
        }
        break;

    case SshConnectingError:
        ON_ENTRY {
            m_connectRequested = false;
            m_connectOptions = NoConnectOption;
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
            m_connectOptions = NoConnectOption;
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

void MerConnection::createConnection()
{
    QTC_CHECK(m_connection == 0);

    SshConnectionParameters params(m_params);
    params.timeout = SSH_TRY_CONNECT_TIMEOUT / 1000;
    m_connection = new SshConnection(params, this);
    connect(m_connection.data(), &SshConnection::connected,
            this, &MerConnection::onSshConnected);
    connect(m_connection.data(), &SshConnection::disconnected,
            this, &MerConnection::onSshDisconnected);
    connect(m_connection.data(), &SshConnection::error,
            this, &MerConnection::onSshError);
}

void MerConnection::vmWantFastPollState(bool want)
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

void MerConnection::vmPollState(Synchronization synchronization)
{
    if (m_pollingVmState) {
        if (synchronization == Synchronous) {
            DBG << "Already polling - waiting";
            waitForVmPollStateFinish();
        } else {
            DBG << "Already polling";
        }
        return;
    }

    m_pollingVmState = true;

    QEventLoop *loop = 0;
    if (synchronization == Synchronous)
        loop = new QEventLoop(this);

    auto handler = [this, loop](bool vmRunning, bool vmExists) {
        bool changed = false;

        if (vmRunning != m_cachedVmRunning) {
            DBG << "VM running:" << m_cachedVmRunning << "-->" << vmRunning;
            m_cachedVmRunning = vmRunning;
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

    MerVirtualBoxManager::isVirtualMachineRunning(m_vmName, this, handler);

    if (synchronization == Synchronous) {
        loop->exec();
        delete loop, loop = 0;
    }
}

void MerConnection::waitForVmPollStateFinish()
{
    while (m_pollingVmState)
        QCoreApplication::processEvents();
}

void MerConnection::sshTryConnect()
{
    if (!m_connection || (m_connection->state() == SshConnection::Unconnected &&
                /* Important: retry only after an SSH connection error is reported to us! Otherwise
                 * we would end trying-again endlessly, suppressing any SSH error. */
                m_cachedSshError != SshNoError && m_cachedSshErrorOrigin == m_connection)) {
        if (m_connection)
            DBG << "SSH try connect - previous error:" << m_cachedSshError << m_cachedSshErrorString;
        else
            DBG << "SSH try connect - no active connection";
        delete m_connection;
        createConnection();
        m_connection->connectToHost();
    }
}

bool MerConnection::isRecoverable(QSsh::SshError sshError)
{
    switch (sshError) {
    case SshNoError:             return true;
    case SshSocketError:         return true;
    case SshTimeoutError:        return true;
    case SshProtocolError:       return true;
    case SshHostKeyError:        return false;
    case SshKeyFileError:        return false;
    case SshAuthenticationError: return false;
    case SshClosedByServerError: return true;
    case SshAgentError:          return false;
    case SshInternalError:       return true;
    }

    QTC_CHECK(false);
    return false;
}

const char *MerConnection::str(State state)
{
    static const char *strings[] = {
        "Disconnected",
        "StartingVm",
        "Connecting",
        "Error",
        "Connected",
        "Disconnecting",
        "ClosingVm",
    };

    return strings[state];
}

const char *MerConnection::str(VmState vmState)
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

const char *MerConnection::str(SshState sshState)
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

void MerConnection::vmStmScheduleExec()
{
    m_vmStmExecTimer.start(0, this);
}

void MerConnection::sshStmScheduleExec()
{
    m_sshStmExecTimer.start(0, this);
}

void MerConnection::onSshConnected()
{
    DBG << "SSH connected";
    m_cachedSshConnected = true;
    m_cachedSshError = SshNoError;
    m_cachedSshErrorString.clear();
    sshStmScheduleExec();
}

void MerConnection::onSshDisconnected()
{
    DBG << "SSH disconnected";
    m_cachedSshConnected = false;
    vmPollState(Asynchronous);
    sshStmScheduleExec();
}

void MerConnection::onSshError(SshError error)
{
    DBG << "SSH error:" << error << m_connection->errorString();
    m_cachedSshError = error;
    m_cachedSshErrorString = m_connection->errorString();
    m_cachedSshErrorOrigin = m_connection;
    vmPollState(Asynchronous);
    sshStmScheduleExec();
}

void MerConnection::onRemoteShutdownProcessFinished()
{
    DBG << "Remote shutdown process finished. Succeeded:" << !m_remoteShutdownProcess->isError();
    vmStmScheduleExec();
}

void MerConnection::Ui::ask(Question which, OnStatusChanged onStatusChanged,
        std::function<void()> ifYes, std::function<void()> ifNo)
{
    switch (status(which)) {
    case Ui::NotAsked:
        ask(which, onStatusChanged);
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

#ifdef MER_LIBRARY
void MerConnectionWidgetUi::warn(Warning which)
{
    switch (which) {
    case Ui::AlreadyConnecting:
        openWarningBox(tr("Already Connecting to Virtual Machine"),
                tr("Already connecting to the \"%1\" virtual machine - please repeat later."))
            ->setAttribute(Qt::WA_DeleteOnClose);
        break;
    case Ui::AlreadyDisconnecting:
        openWarningBox(tr("Already Disconnecting from Virtual Machine"),
                tr("Already disconnecting from the \"%1\" virtual machine - please repeat later."))
            ->setAttribute(Qt::WA_DeleteOnClose);
        break;
    case Ui::UnableToCloseVm:
        QTC_CHECK(!m_unableToCloseVmWarningBox);
        m_unableToCloseVmWarningBox = openWarningBox(
                tr("Unable to Close Virtual Machine"),
                tr("Timeout waiting for the \"%1\" virtual machine to close."));
        break;
    case Ui::VmNotRegistered:
        openWarningBox(tr("Virtual Machine Not Found"),
                tr("No virtual machine with the name \"%1\" found. Check your installation."))
            ->setAttribute(Qt::WA_DeleteOnClose);
        break;
    }
}

void MerConnectionWidgetUi::dismissWarning(Warning which)
{
    switch (which) {
    case Ui::UnableToCloseVm:
        deleteDialog(m_unableToCloseVmWarningBox);
        break;
    default:
        QTC_CHECK(false);
    }
}

bool MerConnectionWidgetUi::shouldAsk(Question which) const
{
    switch (which) {
    case Ui::StartVm:
        return MerSettings::isAskBeforeStartingVmEnabled();
        break;
    default:
        QTC_CHECK(false);
        return false;
    }
}

void MerConnectionWidgetUi::ask(Question which, OnStatusChanged onStatusChanged)
{
    switch (which) {
    case Ui::StartVm:
        QTC_CHECK(!m_startVmQuestionBox);
        m_startVmQuestionBox = openQuestionBox(
                onStatusChanged,
                tr("Start Virtual Machine"),
                tr("The \"%1\" virtual machine is not running. Do you want to start it now?"),
                QString(),
                MerSettings::isAskBeforeStartingVmEnabled()
                    ? [] { MerSettings::setAskBeforeStartingVmEnabled(false); }
                    : std::function<void()>());
        break;
    case Ui::ResetVm:
        QTC_CHECK(!m_resetVmQuestionBox);
        bool startedOutside;
        connection()->isVirtualMachineOff(0, &startedOutside);
        m_resetVmQuestionBox = openQuestionBox(
                onStatusChanged,
                tr("Reset Virtual Machine"),
                tr("Connection to the \"%1\" virtual machine failed recently. "
                    "Do you want to reset the virtual machine first?"),
                startedOutside ?
                    tr("This virtual machine has been started outside of this Qt Creator session.")
                    : QString());
        break;
    case Ui::CloseVm:
        QTC_CHECK(!m_closeVmQuestionBox);
        m_closeVmQuestionBox = openQuestionBox(
                onStatusChanged,
                tr("Close Virtual Machine"),
                tr("Do you really want to close the \"%1\" virtual machine?"),
                tr("This virtual machine has been started outside of this Qt Creator session. "
                    "Answer \"No\" to disconnect and leave the virtual machine running."));
        break;
    case Ui::CancelConnecting:
        QTC_CHECK(!m_connectingProgressDialog);
        m_connectingProgressDialog = openProgressDialog(
                onStatusChanged,
                tr("Connecting to Virtual Machine"),
                tr("Connecting to the \"%1\" virtual machine…"));
        break;
    case Ui::CancelLockingDown:
        QTC_CHECK(!m_lockingDownProgressDialog);
        m_lockingDownProgressDialog = openProgressDialog(
                onStatusChanged,
                tr("Closing Virtual Machine"),
                tr("Waiting for the \"%1\" virtual machine to close…"));
        break;
    }
}

void MerConnectionWidgetUi::dismissQuestion(Question which)
{
    switch (which) {
    case Ui::StartVm:
        deleteDialog(m_startVmQuestionBox);
        break;
    case Ui::ResetVm:
        deleteDialog(m_resetVmQuestionBox);
        break;
    case Ui::CloseVm:
        deleteDialog(m_closeVmQuestionBox);
        break;
    case Ui::CancelConnecting:
        deleteDialog(m_connectingProgressDialog);
        break;
    case Ui::CancelLockingDown:
        deleteDialog(m_lockingDownProgressDialog);
        break;
    }
}

MerConnection::Ui::QuestionStatus MerConnectionWidgetUi::status(Question which) const
{
    switch (which) {
    case Ui::StartVm:
        return status(m_startVmQuestionBox);
        break;
    case Ui::ResetVm:
        return status(m_resetVmQuestionBox);
        break;
    case Ui::CloseVm:
        return status(m_closeVmQuestionBox);
        break;
    case Ui::CancelConnecting:
        return status(m_connectingProgressDialog);
        break;
    case Ui::CancelLockingDown:
        return status(m_lockingDownProgressDialog);
        break;
    }

    QTC_CHECK(false);
    return NotAsked;
}

QMessageBox *MerConnectionWidgetUi::openWarningBox(const QString &title, const QString &text)
{
    QMessageBox *box = new QMessageBox(
            QMessageBox::Warning,
            title,
            text.arg(connection()->virtualMachine()),
            QMessageBox::Ok,
            ICore::mainWindow());
    box->show();
    box->raise();
    return box;
}


QMessageBox *MerConnectionWidgetUi::openQuestionBox(OnStatusChanged onStatusChanged,
        const QString &title, const QString &text, const QString &informativeText,
        std::function<void()> setDoNotAskAgain)
{
    QMessageBox *box = new QMessageBox(
            QMessageBox::Question,
            title,
            text.arg(connection()->virtualMachine()),
            QMessageBox::Yes | QMessageBox::No,
            ICore::mainWindow());
    box->setInformativeText(informativeText);
    if (setDoNotAskAgain) {
        box->setCheckBox(new QCheckBox(CheckableMessageBox::msgDoNotAskAgain()));
        connect(box, &QMessageBox::finished, [=] {
            if (box->checkBox()->isChecked())
                setDoNotAskAgain();
        });
    }
    box->setEscapeButton(QMessageBox::No);
    connect(box, &QMessageBox::finished,
            connection(), onStatusChanged);
    box->show();
    box->raise();
    return box;
}

QProgressDialog *MerConnectionWidgetUi::openProgressDialog(OnStatusChanged onStatusChanged,
        const QString &title, const QString &text)
{
    QProgressDialog *dialog = new QProgressDialog(ICore::mainWindow());
    dialog->setMaximum(0);
    dialog->setWindowTitle(title);
    dialog->setLabelText(text.arg(connection()->virtualMachine()));
    connect(dialog, &QDialog::finished,
            connection(), onStatusChanged);
    dialog->show();
    dialog->raise();
    return dialog;
}

template<class Dialog>
void MerConnectionWidgetUi::deleteDialog(QPointer<Dialog> &dialog)
{
    if (!dialog)
        return;

    if (!dialog->isVisible()) {
        delete dialog;
    } else {
        dialog->setEnabled(false);
        QTimer::singleShot(DISMISS_MESSAGE_BOX_DELAY, dialog.data(), &QObject::deleteLater);
        dialog->disconnect();
        dialog = 0;
    }
}

MerConnection::Ui::QuestionStatus MerConnectionWidgetUi::status(QMessageBox *box) const
{
    if (!box)
        return NotAsked;
    else if (QAbstractButton *button = box->clickedButton())
        return button == box->button(QMessageBox::Yes) ? Yes : No;
    else
        return Asked;
}

MerConnection::Ui::QuestionStatus MerConnectionWidgetUi::status(QProgressDialog *dialog) const
{
    if (!dialog)
        return NotAsked;
    else if (!dialog->isVisible())
        return dialog->wasCanceled() ? Yes : No;
    else
        return Asked;
}
#endif // MER_LIBRARY

} // Mer

#include "merconnection.moc"
