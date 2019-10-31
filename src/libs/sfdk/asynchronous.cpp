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

namespace Sfdk {

namespace {
const int TERMINATE_TIMEOUT_MS = 3000;

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

    void run() override
    {
        QTimer::singleShot(0, this, [=]() {
            if (m_context)
                m_functor();
            emit success();
            emit done(true);
        });
    }

    QDebug print(QDebug debug) const override
    {
        return debug;
    }

public slots:
    void terminate() override
    {
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

/*!
 * \class CommandQueue
 * \internal
 */

CommandQueue::~CommandQueue()
{
    QTC_CHECK(m_queue.empty());
}

void CommandQueue::wait()
{
    if (!m_queue.empty()) {
        QEventLoop loop;
        connect(this, &CommandQueue::empty, &loop, &QEventLoop::quit);
        loop.exec();
    }
}

CommandQueue::BatchId CommandQueue::beginBatch()
{
    m_lastBatchId++;
    if (m_lastBatchId < 0)
        m_lastBatchId = 1;
    m_currentBatchId = m_lastBatchId;
    return m_currentBatchId;
}

void CommandQueue::endBatch()
{
    m_currentBatchId = -1;
}

void CommandQueue::cancelBatch(BatchId batchId)
{
    while (!m_queue.empty() && m_queue.front().first == batchId) {
        CommandRunner *runner = m_queue.front().second.get();
        qCDebug(vmsQueue) << "Canceled" << runner;
        m_queue.pop_front();
    }
}

void CommandQueue::enqueue(std::unique_ptr<CommandRunner> &&runner)
{
    QTC_ASSERT(runner, return);
    qCDebug(vmsQueue) << "Enqueued" << runner.get();
    m_queue.emplace_back(m_currentBatchId, std::move(runner));
    scheduleDequeue();
}

void CommandQueue::enqueueImmediate(std::unique_ptr<CommandRunner> &&runner)
{
    QTC_ASSERT(runner, return);
    qCDebug(vmsQueue) << "Enqueued (immediate)" << runner.get();
    m_queue.emplace_front(m_currentBatchId, std::move(runner));
    scheduleDequeue();
}

void CommandQueue::enqueueCheckPoint(const QObject *context, const Functor<> &functor)
{
    QTC_ASSERT(context, return);
    QTC_ASSERT(functor, return);
    auto runner = std::make_unique<CheckPointRunner>(context, functor, this);
    qCDebug(vmsQueue) << "Enqueued check point" << runner.get();
    m_queue.emplace_back(m_currentBatchId, std::move(runner));
    scheduleDequeue();
}

void CommandQueue::enqueueImmediateCheckPoint(const QObject *context, const Functor<> &functor)
{
    QTC_ASSERT(context, return);
    QTC_ASSERT(functor, return);
    auto runner = std::make_unique<CheckPointRunner>(context, functor, this);
    qCDebug(vmsQueue) << "Enqueued check point (immediate)" << runner.get();
    m_queue.emplace_front(m_currentBatchId, std::move(runner));
    scheduleDequeue();
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

void CommandQueue::scheduleDequeue()
{
    m_dequeueTimer.start(0, this);
}

void CommandQueue::dequeue()
{
    if (m_current)
        return;
    if (m_queue.empty()) {
        emit empty();
        return;
    }

    m_current = std::move(m_queue.front().second);
    m_queue.pop_front();

    qCDebug(vmsQueue) << "Dequeued" << m_current.get();

    connect(m_current.get(), &CommandRunner::done,
            this, &CommandQueue::finalize);

    m_current->run();
}

void CommandQueue::finalize()
{
    QTC_ASSERT(sender() == m_current.get(), return);
    qCDebug(vmsQueue) << "Finished" << m_current.get();

    m_current->disconnect(this);
    m_current->deleteLater();
    m_current.release();

    scheduleDequeue();
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

void ProcessRunner::run()
{
    m_process->start(QIODevice::ReadWrite | QIODevice::Text);
}

QDebug ProcessRunner::print(QDebug debug) const
{
    debug << m_process->program() << m_process->arguments();
    return debug.maybeSpace();
}

void ProcessRunner::terminate()
{
    QTC_CHECK(m_process->state() != QProcess::Starting);
    if (m_process->state() == QProcess::NotRunning)
        return;

    m_crashExpected = true;

    m_process->terminate();
    if (!m_terminateTimeoutTimer.isActive())
        m_terminateTimeoutTimer.start(TERMINATE_TIMEOUT_MS, this);
}

void ProcessRunner::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == m_terminateTimeoutTimer.timerId()) {
        m_terminateTimeoutTimer.stop();
        // Note that on Windows it always ends here as terminate() has no
        // effect on VBoxManage there
        m_process->kill();
    } else {
        QObject::timerEvent(event);
    }
}

void ProcessRunner::onErrorOccured(QProcess::ProcessError error)
{
    if (error == QProcess::FailedToStart) {
        qCWarning(vms) << "VBoxManage failed to start";
        emit failure();
        emit done(false);
    }
}

void ProcessRunner::onFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    m_terminateTimeoutTimer.stop();

    if (exitStatus != QProcess::NormalExit) {
        if (m_crashExpected) {
            qCDebug(vms) << "VBoxManage crashed as expected. Arguments:" << m_process->arguments();
            emit success();
            emit done(true);
            return;
        }
        qCWarning(vms) << "VBoxManage crashed. Arguments:" << m_process->arguments();
        emit failure();
        emit done(false);
    } else if (!m_expectedExitCodes.contains(exitCode)) {
        qCWarning(vms) << "VBoxManage exited with unexpected exit code" << exitCode
            << ". Arguments:" << m_process->arguments();
        emit failure();
        emit done(false);
    } else {
        emit success();
        emit done(true);
    }
}

} // namespace Sfdk

#include "asynchronous.moc"
