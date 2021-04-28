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

#include "asynchronous_p.h"

#include <utils/qtcassert.h>

#include <QTimer>
#include <QTimerEvent>

namespace Sfdk {

namespace {

const int INDENT_WIDTH = 4;

class CheckPointRunner : public CommandRunner
{
    Q_OBJECT

public:
    explicit CheckPointRunner(const QObject *context, const Functor<> &functor, QObject *parent = 0)
        : CommandRunner(parent)
        , m_context(context)
        , m_functor(functor)
    {
    }

    void doRun() override
    {
        QTimer::singleShot(0, this, [=]() {
            if (m_context)
                m_functor();
            emitDone(true);
        });
    }

    QDebug print(QDebug debug) const override
    {
        debug << "CHECK POINT";
        return debug;
    }

private:
    const QPointer<const QObject> m_context;
    const Functor<> m_functor;
};

} // namespace anonymous

/*!
 * \class CommandRunner
 * \internal
 */

void CommandRunner::run()
{
    doRun();
    emit started(QPrivateSignal());
}

void CommandRunner::emitDone(bool ok)
{
    if (ok)
        emit success(QPrivateSignal());
    else
        emit failure(QPrivateSignal());

    emit done(ok, QPrivateSignal());
}

/*!
 * \class CommandQueue
 * \internal
 */

CommandQueue::CommandQueue(const QString &name, int depth, QObject *parent)
    : QObject(parent)
    , m_name(name)
    , m_depth(depth)
{
}

CommandQueue::~CommandQueue()
{
    QTC_CHECK(m_queue.empty());
    QTC_CHECK(!m_current);
}

void CommandQueue::run()
{
    m_running = true;
    scheduleDequeue();
}

bool CommandQueue::isEmpty() const
{
    return m_queue.empty() && !m_current;
}

void CommandQueue::cancel()
{
    if (m_queue.empty())
        return;
    while (!m_queue.empty()) {
        CommandRunner *runner = m_queue.front().get();
        qCDebug(queue) << "Canceled" << qPrintable(printIndent()) << runner;
        m_queue.pop_front();
    }
    if (!m_current)
        emit empty();
}

void CommandQueue::enqueue(std::unique_ptr<CommandRunner> &&runner)
{
    QTC_ASSERT(runner, return);
    qCDebug(queue) << "Enqueued" << qPrintable(printIndent()) << runner.get();
    m_queue.emplace_back(std::move(runner));
    scheduleDequeue();
}

void CommandQueue::enqueueCheckPoint(const QObject *context, const Functor<> &functor)
{
    QTC_ASSERT(context, return);
    QTC_ASSERT(functor, return);
    enqueue(std::make_unique<CheckPointRunner>(context, functor));
}

void CommandQueue::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == m_dequeueTimer.timerId()) {
        m_dequeueTimer.stop();
        dequeue();
    } else  {
        QObject::timerEvent(event);
    }
}

QString CommandQueue::printIndent() const
{
    return QString(m_depth * INDENT_WIDTH, ' ');
}

void CommandQueue::scheduleDequeue()
{
    if (!m_running)
        return;
    m_dequeueTimer.start(0, this);
}

void CommandQueue::dequeue()
{
    if (m_current)
        return;
    if (m_queue.empty())
        return;

    m_current = std::move(m_queue.front());
    m_queue.pop_front();

    qCDebug(queue) << "Dequeued" << qPrintable(printIndent()) << m_current.get();

    connect(m_current.get(), &CommandRunner::done,
            this, &CommandQueue::finalize);

    m_current->run();
}

void CommandQueue::finalize(bool ok)
{
    QTC_ASSERT(sender() == m_current.get(), return);
    qCDebug(queue) << "Finished" << qPrintable(printIndent()) << m_current.get();

    m_current->disconnect(this);
    m_current->deleteLater();
    m_current.release();

    if (!ok)
        failure();

    if (m_queue.empty())
        emit empty();

    scheduleDequeue();
}

/*!
 * \class BatchRunner
 * \internal
 */

BatchRunner::BatchRunner(const QString &name, int depth, QObject *parent)
    : CommandRunner(parent)
    , m_queue(std::make_unique<CommandQueue>(name, depth, this))
{
    connect(m_queue.get(), &CommandQueue::empty, this, &BatchRunner::onEmpty);
    connect(m_queue.get(), &CommandQueue::failure, this, &BatchRunner::onFailure);
}

BatchRunner::~BatchRunner()
{
    QTC_CHECK(m_queue->isEmpty());
}

void BatchRunner::doRun()
{
    m_queue->run();
}

void BatchRunner::finish(bool ok)
{
    QTC_ASSERT(m_queue->isEmpty(), m_queue->cancel());
    emitDone(ok);
}

QDebug BatchRunner::print(QDebug debug) const
{
    QDebugStateSaver saver(debug);
    debug.nospace().noquote() << '[' << m_queue->name() << ']';
    return debug;
}

void BatchRunner::onEmpty()
{
    if (m_autoFinish)
        finish(true);
}

void BatchRunner::onFailure()
{
    if (m_propagateFailure) {
        m_queue->cancel();
        finish(false);
    }
}

/*!
 * \class BatchComposer
 * \internal
 */

std::stack<BatchRunner *> BatchComposer::s_stack;

BatchComposer::BatchComposer(BatchRunner *batch)
    : m_batch(batch)
{
    s_stack.push(m_batch);
}

BatchComposer::BatchComposer(BatchComposer &&other)
    : m_batch(std::exchange(other.m_batch, nullptr))
{
}

BatchComposer BatchComposer::createBatch(const QString &batchName)
{
    auto batch = std::make_unique<BatchRunner>(batchName, top()->depth() + 1);
    BatchRunner *const batch_ = batch.get();
    top()->enqueue(std::move(batch));
    return BatchComposer(batch_);
}

BatchComposer BatchComposer::extendBatch(BatchRunner *batch)
{
    const QString indent((batch->queue()->depth() - 1) * INDENT_WIDTH, ' ');
    qCDebug(queue) << "Reopened" << qPrintable(indent) << batch;

    return BatchComposer(batch);
}

BatchComposer::~BatchComposer()
{
    if (!m_batch) // moved
        return;

    QTC_ASSERT(!s_stack.empty(), return);
    QTC_ASSERT(s_stack.top() == m_batch, return);
    s_stack.pop();
}

void BatchComposer::enqueue(std::unique_ptr<CommandRunner> &&runner)
{
    top()->enqueue(std::move(runner));
}

void BatchComposer::enqueueCheckPoint(const QObject *context, const Functor<> &functor)
{
    top()->enqueueCheckPoint(context, functor);
}

CommandQueue *BatchComposer::top()
{
    return s_stack.empty()
        ? SdkPrivate::commandQueue()
        : s_stack.top()->queue();
}

/*!
 * \class ProcessRunner
 * \internal
 */

ProcessRunner::ProcessRunner(const QString &program, const QStringList &arguments, QObject *parent)
    : CommandRunner(parent)
    , m_process(std::make_unique<QProcess>(this))
{
    m_process->setProcessChannelMode(QProcess::ForwardedErrorChannel);
    m_process->setProgram(program);
    m_process->setArguments(arguments);
    connect(m_process.get(), &QProcess::errorOccurred,
            this, &ProcessRunner::onErrorOccured);
    connect(m_process.get(), QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &ProcessRunner::onFinished);
}

void ProcessRunner::doRun()
{
    m_process->start(QIODevice::ReadWrite | QIODevice::Text);
}

QDebug ProcessRunner::print(QDebug debug) const
{
    debug << m_process->program() << m_process->arguments();
    return debug;
}

void ProcessRunner::onErrorOccured(QProcess::ProcessError error)
{
    if (error == QProcess::FailedToStart) {
        qCWarning(vms) << "Process" << m_process->program() << "failed to start";
        emitDone(false);
    }
}

void ProcessRunner::onFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitStatus != QProcess::NormalExit) {
        qCWarning(vms) << "Process" << m_process->program() << " crashed. Arguments:"
            << m_process->arguments();
        emitDone(false);
    } else if (!m_expectedExitCodes.isEmpty() && !m_expectedExitCodes.contains(exitCode)) {
        qCWarning(vms) << "Process" << m_process->program() << " exited with unexpected exit code"
            << exitCode << ". Arguments:" << m_process->arguments();
        emitDone(false);
    } else {
        emitDone(true);
    }
}

} // namespace Sfdk

#include "asynchronous.moc"
