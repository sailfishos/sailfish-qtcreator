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

#pragma once

#include <memory>

#include <QObject>
#include <QSet>

namespace Sfdk {

class Task : public QObject
{
    Q_OBJECT

public:
    enum State {
        NotStarted,
        Running,
        Stopping,
        Stopped,
        Continuing,
        ContinuingBeforeTerminating,
        Terminating,
        TerminatingBeforeContinuing,
        MaybeTerminated,
        Error,
    };
    Q_ENUM(State)

    enum Event {
        NoEvent = 1<<0,
        Started = 1<<1,
        Terminate = 1<<2,
        TerminateSuccess = 1<<3,
        TerminateFailure = 1<<4,
        Stop = 1<<5,
        StopSuccess = 1<<6,
        StopFailure = 1<<7,
        Continue = 1<<8,
        ContinueSuccess = 1<<9,
        ContinueFailure = 1<<10,
        Exited = 1<<11,
    };
    Q_ENUM(Event)
    Q_DECLARE_FLAGS(Events, Event)

public:
    using QObject::QObject;
    ~Task() override;

protected:
    void started();
    virtual void beginTerminate() = 0;
    virtual void beginStop() = 0;
    virtual void beginContinue() = 0;
    void endTerminate(bool ok);
    void endStop(bool ok);
    void endContinue(bool ok);
    void exited();

private:
    friend class TaskManager;
    void post(Event event);

    Q_INVOKABLE void process(Sfdk::Task::Event event);
    void transition(State toState);
    void internalTransition();
    bool exiting();
    bool entering();
    bool step();

private:
    State state = NotStarted;
    bool onTransition = false;
    bool processing = false;
    Event event = NoEvent;
};

class SignalHandler;
class TaskManager
{
    Q_GADGET

public:
    enum Request {
        Terminate,
        Stop,
        Continue,
    };
    Q_ENUM(Request)

public:
    TaskManager();
    ~TaskManager();

private:
    friend class Task;
    friend class SignalHandler;
    static void process(Request request);

    static void started(Task *task);
    static void changed(Task *task);
    static void exited(Task *task);

    static void stop();

private:
    static TaskManager *s_instance;
    std::unique_ptr<SignalHandler> signalHandler;
    QSet<Task *> tasks;
};

} // namespace Sfdk

Q_DECLARE_OPERATORS_FOR_FLAGS(Sfdk::Task::Events)
