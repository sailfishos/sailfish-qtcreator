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

#include <QBasicTimer>
#include <QList>
#include <QObject>
#include <QProcess>

#include <deque>
#include <memory>

namespace Sfdk {

class CommandRunner : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;

    virtual void run() = 0;

    virtual QDebug print(QDebug debug) const = 0;

public slots:
    virtual void terminate() = 0;

signals:
    void success();
    void failure();
    void done(bool ok);
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
    using BatchId = int;

    using QObject::QObject;
    ~CommandQueue() override;

    void wait();

    BatchId beginBatch();
    void endBatch();
    void cancelBatch(BatchId batchId);
    void enqueue(std::unique_ptr<CommandRunner> &&runner);
    void enqueueImmediate(std::unique_ptr<CommandRunner> &&runner);

signals:
    void empty();

protected:
    void timerEvent(QTimerEvent *event) override;

private:
    void scheduleDequeue();
    void dequeue();

private slots:
    void finalize();

private:
    std::deque<std::pair<BatchId, std::unique_ptr<CommandRunner>>> m_queue;
    BatchId m_lastBatchId = 0;
    BatchId m_currentBatchId = -1;
    std::unique_ptr<CommandRunner> m_current;
    QBasicTimer m_dequeueTimer;
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

    void run() override;

    QDebug print(QDebug debug) const override;

public slots:
    void terminate() override;

protected:
    void timerEvent(QTimerEvent *event) override;

private slots:
    void onErrorOccured(QProcess::ProcessError error);
    void onFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    const std::unique_ptr<QProcess> m_process;
    QList<int> m_expectedExitCodes = {0};
    bool m_crashExpected = false;
    QBasicTimer m_terminateTimeoutTimer;
};

} // namespace Sfdk
