/****************************************************************************
**
** Copyright (C) 2019 Jolla Ltd.
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

#include "asynchronous.h"

#include "sdk_p.h"

#include <ssh/sshremoteprocessrunner.h>
#include <utils/optional.h>

#include <QBasicTimer>
#include <QList>
#include <QObject>
#include <QProcess>

#include <deque>
#include <memory>
#include <stack>

namespace Sfdk {

class CommandRunner : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;

    void run();

    virtual QDebug print(QDebug debug) const = 0;

protected:
    virtual void doRun() = 0;
    void emitDone(bool ok);

signals:
    void started(QPrivateSignal);
    void success(QPrivateSignal);
    void failure(QPrivateSignal);
    void done(bool ok, QPrivateSignal);
};

inline QDebug operator<<(QDebug debug, const CommandRunner *runner)
{
    debug << (void*)runner;
    return runner->print(debug);
}

class CommandQueue : public QObject
{
    Q_OBJECT

public:
    CommandQueue(const QString &name, int depth, QObject *parent = nullptr);
    ~CommandQueue() override;

    QString name() const { return m_name; }
    int depth() const { return m_depth; }

    void run();
    bool isEmpty() const;
    void cancel();

    void enqueue(std::unique_ptr<CommandRunner> &&runner);
    void enqueueCheckPoint(const QObject *context, const Functor<> &functor);

signals:
    void empty();
    void failure();

protected:
    void timerEvent(QTimerEvent *event) override;

private:
    QString printIndent() const;
    void scheduleDequeue();
    void dequeue();

private slots:
    void finalize(bool ok);

private:
    const QString m_name;
    const int m_depth = 0;
    std::deque<std::unique_ptr<CommandRunner>> m_queue;
    bool m_running = false;
    std::unique_ptr<CommandRunner> m_current;
    QBasicTimer m_dequeueTimer;
};

class BatchRunner : public CommandRunner
{
    Q_OBJECT

public:
    BatchRunner(const QString &name, int depth, QObject *parent = nullptr);
    ~BatchRunner() override;

    CommandQueue *queue() const { return m_queue.get(); }

    void setAutoFinish(bool autoFinish) { m_autoFinish = autoFinish; }
    void setPropagateFailure(bool propagateFailure) { m_propagateFailure = propagateFailure; }

    void finish(bool ok);

    QDebug print(QDebug debug) const override;

protected:
    void doRun() override;

private:
    void doFinish(bool ok);

private slots:
    void onEmpty();
    void onFailure();

private:
    const std::unique_ptr<CommandQueue> m_queue;
    bool m_autoFinish = true;
    bool m_propagateFailure = false;
    Utils::optional<bool> m_explicitResult;
};

class BatchComposer
{
public:
    BatchComposer(BatchComposer &&other);
    ~BatchComposer();

    Q_REQUIRED_RESULT static BatchComposer createBatch(const QString &batchName);
    Q_REQUIRED_RESULT static BatchComposer extendBatch(BatchRunner *batch);

    BatchRunner *batch() const { return m_batch; }

    static void enqueue(std::unique_ptr<CommandRunner> &&runner);
    static void enqueueCheckPoint(const QObject *context, const Functor<> &functor);

    template<typename Runner, typename... Args>
    static Runner *enqueue(Args &&...args);

private:
    explicit BatchComposer(BatchRunner *batch);
    Q_DISABLE_COPY(BatchComposer)

    static CommandQueue *top();

private:
    static std::stack<BatchRunner *> s_stack;
    BatchRunner *m_batch;
};

class ProcessRunner : public CommandRunner
{
    Q_OBJECT

public:
    explicit ProcessRunner(const QString &program, const QStringList &arguments,
            QObject *parent = 0);

    QProcess *process() const { return m_process.get(); }

    QList<int> expectedExitCodes() const { return m_expectedExitCodes; }
    void setExpectedExitCodes(const QList<int> &expectedExitCodes)
    {
        m_expectedExitCodes = expectedExitCodes;
    }

    QDebug print(QDebug debug) const override;

protected:
    void doRun() override;

private slots:
    void onErrorOccured(QProcess::ProcessError error);
    void onFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    const std::unique_ptr<QProcess> m_process;
    QList<int> m_expectedExitCodes = {0};
    bool m_crashExpected = false;
};

class RemoteProcessRunner : public CommandRunner
{
    Q_OBJECT

public:
    RemoteProcessRunner(const QString &displayName, const QString &command,
            const QSsh::SshConnectionParameters &sshParameters, QObject *parent = nullptr);

    QSsh::SshRemoteProcessRunner *sshRunner() const { return m_sshRunner.get(); }

    QList<int> expectedExitCodes() const { return m_expectedExitCodes; }
    void setExpectedExitCodes(const QList<int> &expectedExitCodes)
    {
        m_expectedExitCodes = expectedExitCodes;
    }

    QDebug print(QDebug debug) const override;

protected:
    void doRun() override;

private slots:
    void onProcessClosed();
    void onConnectionError();

private:
    const std::unique_ptr<QSsh::SshRemoteProcessRunner> m_sshRunner;
    const QString m_displayName;
    const QString m_command;
    const QSsh::SshConnectionParameters m_sshParamaters;
    QList<int> m_expectedExitCodes = {0};
};

template<typename Runner, typename... Args>
Runner *BatchComposer::enqueue(Args &&...args)
{
    auto runner = std::make_unique<Runner>(std::forward<Args>(args)...);
    Runner *const runner_ = runner.get();
    enqueue(std::move(runner));
    return runner_;
}

} // namespace Sfdk
