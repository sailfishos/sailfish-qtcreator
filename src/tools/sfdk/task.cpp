/****************************************************************************
**
** Copyright (C) 2019 Jolla Ltd.
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

#include "task.h"

#include "sfdkglobal.h"
#include "textutils.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QCoreApplication>
#include <QScopedValueRollback>

#ifdef Q_OS_WIN
# include <windows.h>
#else
# include <QSocketNotifier>
# include <signal.h>
# include <sys/socket.h>
# include <unistd.h>
#endif

using namespace Sfdk;

namespace Sfdk {

class SignalHandler
{
    Q_DECLARE_TR_FUNCTIONS(Sfdk::SignalHandler)

public:
    SignalHandler();
    ~SignalHandler();

    void setUp();
    void restore();

    static SignalHandler *instance;

#ifdef Q_OS_WIN
    static BOOL WINAPI ctrlHandler(DWORD fdwCtrlType);
#else
    static void signalHandler(int signum);
    int signalFd[2] = {-1, -1};
    std::unique_ptr<QSocketNotifier> signalNotifier;
#endif
};

} // namespace Sfdk

/*!
 * \class Task
 */

Task::~Task()
{
    bool exited = !TaskManager::s_instance->tasks.contains(this);
    QTC_CHECK(exited);
}

void Task::started()
{
    process(Started);
}

void Task::endTerminate(bool ok)
{
    post(ok ? TerminateSuccess : TerminateFailure);
}

void Task::endStop(bool ok)
{
    post(ok ? StopSuccess : StopFailure);
}

void Task::endContinue(bool ok)
{
    post(ok ? ContinueSuccess : ContinueFailure);
}

void Task::exited()
{
    post(Exited);
    QCoreApplication::sendPostedEvents(this);
}

void Task::post(Event event)
{
    qCDebug(sfdk) << "Task" << this << "posting" << enumName(event);
    // TODO Qt 5.10: use pointer to member, remove Q_INVOKABLE
    QMetaObject::invokeMethod(this, "process", Qt::QueuedConnection, Q_ARG(Sfdk::Task::Event, event));
}

void Task::process(Event event)
{
    QTC_ASSERT(!processing, return);
    QScopedValueRollback<bool>(processing, true);

    bool changed = false;

    this->event = event;
    while (step())
        changed = true;

    if (this->event != NoEvent) {
        qCWarning(sfdk) << "Task: unhandled event" << enumName(event) << "in state"
            << enumName(state);
        this->event = NoEvent;
    }

    if (changed && state != NotStarted)
        TaskManager::changed(this);
}

void Task::transition(State toState)
{
    qCDebug(sfdk).nospace() << "Task " << this << " transition: "
        << enumName(state) << " --" << enumName(event) << "--> " << enumName(toState);

    state = toState;
    onTransition = true;
    event = NoEvent;
}

void Task::internalTransition()
{
    QTC_CHECK(event != NoEvent);
    qCDebug(sfdk) << "Task" << this << "internal transition:" << enumName(state) << "/"
        << enumName(event);
    event = NoEvent;
}

bool Task::exiting()
{
    return onTransition;
}

bool Task::entering()
{
    return std::exchange(onTransition, false);
}

bool Task::step()
{
#define ON_ENTRY if (entering())
#define ON_EXIT if (exiting())
#define TRANSITION(events, toState_) if ((Events(events) & event) && (transition(toState_), true))
#define INTERNAL_TRANSITION(events) if ((Events(events) & event) && (internalTransition(), true))

    static auto sayStopping = []() { qerr() << tr("Stopping...") << endl; };
    static auto sayCleaningUp = []() { qerr() << tr("Cleaning up...") << endl; };
    static auto sayBusy = []() { qerr() << tr("Busy...") << endl; };

    switch (state) {
    case NotStarted:
        ON_ENTRY {
            TaskManager::exited(this);
        }
        TRANSITION(Started, Running) {
            TaskManager::started(this);
        }
        ON_EXIT {}
        break;

    case Running:
        ON_ENTRY {}
        TRANSITION(Stop, Stopping) {}
        TRANSITION(Terminate, Terminating) {}
        TRANSITION(Exited, NotStarted) {}
        ON_EXIT {}
        break;

    case Stopping:
        ON_ENTRY {
            beginStop();
        }
        TRANSITION(StopSuccess, Stopped) {}
        TRANSITION(StopFailure, Error) {}
        INTERNAL_TRANSITION(Stop|Terminate) { sayStopping(); }
        ON_EXIT {}
        break;

    case Stopped:
        ON_ENTRY {}
        TRANSITION(Continue, Continuing) {}
        TRANSITION(Terminate, TerminatingBeforeContinuing) {}
        ON_EXIT {}
        break;

    case Continuing:
        ON_ENTRY {
            beginContinue();
        }
        TRANSITION(ContinueSuccess, Running) {}
        TRANSITION(ContinueFailure, Error) {}
        INTERNAL_TRANSITION(Continue) {}
        INTERNAL_TRANSITION(Stop) { sayBusy(); }
        TRANSITION(Terminate, ContinuingBeforeTerminating) {}
        ON_EXIT {}
        break;

    case ContinuingBeforeTerminating:
        ON_ENTRY {}
        TRANSITION(ContinueSuccess, Terminating) {}
        TRANSITION(ContinueFailure, Error) {}
        INTERNAL_TRANSITION(Stop|Terminate) { sayBusy(); }
        ON_EXIT {}
        break;

    case Terminating:
        ON_ENTRY {
            beginTerminate();
        }
        TRANSITION(TerminateSuccess, MaybeTerminated) {}
        TRANSITION(TerminateFailure, Error) {}
        INTERNAL_TRANSITION(Stop|Terminate) { sayCleaningUp(); }
        ON_EXIT {}
        break;

    case TerminatingBeforeContinuing:
        ON_ENTRY {
            beginTerminate();
        }
        TRANSITION(TerminateSuccess, Continuing) {}
        TRANSITION(TerminateFailure, Error) {}
        // Ignore SIGCONT that arrived after SIGTERM - the order is
        // unpredictable and we would transition to Continuing anyway.
        INTERNAL_TRANSITION(Continue) {}
        INTERNAL_TRANSITION(Stop|Terminate) { sayCleaningUp(); }
        ON_EXIT {}
        break;

    case MaybeTerminated:
        ON_ENTRY {}
        TRANSITION(Exited, NotStarted) {}
        // Allow to repeate that in case it was ignored
        TRANSITION(Terminate, Terminating) {}
        INTERNAL_TRANSITION(Stop) { sayCleaningUp(); }
        ON_EXIT {}
        break;

    case Error:
        ON_ENTRY {}
        TRANSITION(Exited, NotStarted) {}
        ON_EXIT {}
        break;
    }

    return onTransition;

#undef ON_ENTRY
#undef ON_EXIT
#undef TRANSITION
#undef INTERNAL_TRANSITION
}

/*!
 * \class TaskManager
 */

TaskManager *TaskManager::s_instance = nullptr;

TaskManager::TaskManager()
{
    Q_ASSERT(s_instance == nullptr);
    s_instance = this;
    signalHandler = std::make_unique<SignalHandler>();

    // For Task::post(Event)
    qRegisterMetaType<Task::Event>();
}

TaskManager::~TaskManager()
{
    s_instance = nullptr;
}

void TaskManager::process(Request request)
{
    qCDebug(sfdk) << "TaskManager:" << enumName(request) << "requested";

    if (s_instance->tasks.isEmpty()) {
        switch (request) {
        case Terminate:
            break;
        case Stop:
            stop();
            break;
        case Continue:
            break;
        }
        return;
    }

    for (Task *task : qAsConst(s_instance->tasks)) {
        switch (request) {
        case Terminate:
            task->post(Task::Terminate);
            break;
        case Stop:
            task->post(Task::Stop);
            break;
        case Continue:
            task->post(Task::Continue);
            break;
        }
    }
}

void TaskManager::started(Task *task)
{
    QTC_ASSERT(task != nullptr, return);
    QTC_ASSERT(!s_instance->tasks.contains(task), return);

    s_instance->tasks.insert(task);
}

void TaskManager::changed(Task *task)
{
    QTC_ASSERT(task != nullptr, return);
    QTC_ASSERT(s_instance->tasks.contains(task), return);
    Q_UNUSED(task);

    bool allStopped = Utils::allOf(s_instance->tasks,
            [](Task *task) { return task->state == Task::Stopped; });
    if (allStopped)
        stop();
}

void TaskManager::exited(Task *task)
{
    QTC_ASSERT(task != nullptr, return);
    QTC_ASSERT(s_instance->tasks.contains(task), return);

    s_instance->tasks.remove(task);
}

void TaskManager::stop()
{
#ifdef Q_OS_WIN
    QTC_CHECK(false);
#else
    ::kill(::getpid(), SIGSTOP);
#endif
}

/*!
 * \class SignalHandler
 */

SignalHandler *SignalHandler::instance = nullptr;

SignalHandler::SignalHandler()
{
    Q_ASSERT(instance == nullptr);
    instance = this;
    setUp();
}

SignalHandler::~SignalHandler()
{
    instance = nullptr;
    restore();
}

#ifdef Q_OS_WIN
void SignalHandler::setUp()
{
    if (!SetConsoleCtrlHandler(ctrlHandler, TRUE))
        qFatal("Failed to set up console signal handling");
}

void SignalHandler::restore()
{
    if (!SetConsoleCtrlHandler(ctrlHandler, FALSE))
        qFatal("Failed to restore console signal handling");
}

BOOL WINAPI SignalHandler::ctrlHandler(DWORD fdwCtrlType)
{
    switch (fdwCtrlType)
    {
    case CTRL_C_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        TaskManager::process(TaskManager::Terminate);
        return TRUE;

    default:
        return FALSE;
    }
}

#else

void SignalHandler::setUp()
{
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, signalFd) != 0)
        qFatal("Failed to create socket pair for signal handling");

    signalNotifier = std::make_unique<QSocketNotifier>(signalFd[1], QSocketNotifier::Read);
    QObject::connect(signalNotifier.get(), &QSocketNotifier::activated, [this]() {
        signalNotifier->setEnabled(false);

        int signum;
        if (read(signalFd[1], &signum, sizeof(signum)) != sizeof(signum))
            qFatal("Failed to read signal number");

        qCDebug(sfdk) << "Signal handler:"
#ifndef Q_OS_MACOS
            // On macOS it already figures in strsignal output
            << signum
#endif
            << strsignal(signum);

        switch (signum) {
        case SIGHUP:
        case SIGINT:
        case SIGTERM:
            TaskManager::process(TaskManager::Terminate);
            break;

        case SIGTSTP:
            TaskManager::process(TaskManager::Stop);
            break;

        case SIGCONT:
            TaskManager::process(TaskManager::Continue);
            break;

        default:
            qFatal("Read unexpected signal number");
        }

        signalNotifier->setEnabled(true);
    });

    struct sigaction action;
    action.sa_handler = signalHandler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    action.sa_flags |= SA_RESTART;

    if (sigaction(SIGHUP, &action, nullptr))
        qFatal("Failed to set up SIGHUP handling");
    if (sigaction(SIGINT, &action, nullptr))
        qFatal("Failed to set up SIGINT handling");
    if (sigaction(SIGTERM, &action, nullptr))
        qFatal("Failed to set up SIGTERM handling");
    if (sigaction(SIGTSTP, &action, nullptr))
        qFatal("Failed to set up SIGTSTP handling");
    if (sigaction(SIGCONT, &action, nullptr))
        qFatal("Failed to set up SIGCONT handling");
}

void SignalHandler::restore()
{
    if (signal(SIGHUP, SIG_DFL) == SIG_ERR)
        qFatal("Failed to restore original SIGHUP handling");
    if (signal(SIGINT, SIG_DFL) == SIG_ERR)
        qFatal("Failed to restore original SIGINT handling");
    if (signal(SIGTERM, SIG_DFL) == SIG_ERR)
        qFatal("Failed to restore original SIGTERM handling");
    if (signal(SIGTSTP, SIG_DFL) == SIG_ERR)
        qFatal("Failed to restore original SIGTSTP handling");
    if (signal(SIGCONT, SIG_DFL) == SIG_ERR)
        qFatal("Failed to restore original SIGCONT handling");

    close(signalFd[0]), signalFd[0] = -1;
    close(signalFd[1]), signalFd[1] = -1;
}

void SignalHandler::signalHandler(int signum)
{
    if (write(instance->signalFd[0], &signum, sizeof(signum)) != sizeof(signum))
        qFatal("Failed to write signal number");
}

#endif
