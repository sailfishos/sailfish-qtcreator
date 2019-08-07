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

#include "remoteprocess.h"

#include <QEventLoop>
#include <QFile>
#include <QSocketNotifier>
#include <QTimer>

#include <ssh/sshremoteprocessrunner.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include "sfdkconstants.h"
#include "sfdkglobal.h"
#include "textutils.h"

namespace {
const int KILL_RETRY_MS = 400;
const int LINE_BUFFER_MAX = 10240;
const char SIGNAL_TERM[] = "TERM";
const char SIGNAL_STOP[] = "STOP";
const char SIGNAL_CONT[] = "CONT";
}

using namespace QSsh;
using namespace Sfdk;

/*!
 * \class RemoteProcess
 */

RemoteProcess::RemoteProcess(QObject *parent)
    : Task(parent)
    , m_runner(std::make_unique<SshRemoteProcessRunner>(this))
{
    connect(m_runner.get(), &SshRemoteProcessRunner::processStarted,
            this, &RemoteProcess::onProcessStarted);
    connect(m_runner.get(), &SshRemoteProcessRunner::readyReadStandardOutput,
            this, &RemoteProcess::onReadyReadStandardOutput);
    connect(m_runner.get(), &SshRemoteProcessRunner::readyReadStandardError,
            this, &RemoteProcess::onReadyReadStandardError);
    connect(m_runner.get(), &SshRemoteProcessRunner::connectionError,
            this, [this]() { emit connectionError(m_runner->lastConnectionErrorString()); });
}

RemoteProcess::~RemoteProcess()
{
    if (m_stdoutBuffer.hasData())
        emit standardOutput(m_stdoutBuffer.flush());
    if (m_stderrBuffer.hasData())
        emit standardError(m_stderrBuffer.flush());
}

void RemoteProcess::setProgram(const QString& program)
{
    m_program = program;
}

void RemoteProcess::setArguments(const QStringList& arguments)
{
    m_arguments = arguments;
}

void RemoteProcess::setWorkingDirectory(const QString& workingDirectory)
{
    m_workingDirectory = workingDirectory;
}

void RemoteProcess::setSshParameters(const SshConnectionParameters& params)
{
    m_sshConnectionParams = params;
}

void RemoteProcess::setExtraEnvironment(const QProcessEnvironment &extraEnvironment)
{
    m_extraEnvironment = extraEnvironment;
}

void RemoteProcess::setInteractive(bool enabled)
{
    m_interactive = enabled;
}

int RemoteProcess::exec()
{
    QEventLoop loop;
    connect(m_runner.get(), &SshRemoteProcessRunner::connectionError,
            &loop, &QEventLoop::quit);
    connect(m_runner.get(), &SshRemoteProcessRunner::processClosed,
            &loop, &QEventLoop::quit);

    QString fullCommand;
    fullCommand.append("echo $$ && ");
    if (!m_workingDirectory.isEmpty()) {
        fullCommand.append("cd ").append(Utils::QtcProcess::quoteArgUnix(m_workingDirectory))
            .append(" && ");
    }
    fullCommand.append(environmentString(m_extraEnvironment));
    fullCommand.append(" exec ");
    fullCommand.append(Utils::QtcProcess::quoteArgUnix(m_program));
    if (!m_arguments.isEmpty())
        fullCommand.append(' ');
    fullCommand.append(Utils::QtcProcess::joinArgs(m_arguments, Utils::OsTypeLinux));

    m_runner->runInTerminal(fullCommand.toUtf8(), m_sshConnectionParams);

    started();

    loop.exec();

    const int exitCode = m_runner->processExitStatus() == SshRemoteProcess::NormalExit
        ? m_runner->processExitCode()
        : Constants::EXIT_ABNORMAL;

    while (m_killRunner != nullptr)
        QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents);

    exited();

    return exitCode;
}

void RemoteProcess::beginTerminate()
{
    kill(SIGNAL_TERM, &RemoteProcess::endTerminate);
}

void RemoteProcess::beginStop()
{
    kill(SIGNAL_STOP, &RemoteProcess::endStop);
}

void RemoteProcess::beginContinue()
{
    kill(SIGNAL_CONT, &RemoteProcess::endContinue);
}

void RemoteProcess::kill(const QString &signal, void (RemoteProcess::*callback)(bool))
{
    if (m_killRunner != nullptr) {
        qCDebug(sfdk) << "Already trying to terminate the remote process";
        (this->*callback)(false);
        return;
    }

    if (!m_runner->isProcessRunning()) {
        qCDebug(sfdk) << "Remote process is not running";
        (this->*callback)(true);
        return;
    }

    if (m_processId == 0) {
        QTimer::singleShot(KILL_RETRY_MS, this, [=]() { kill(signal, callback); });
        return;
    }

    m_killRunner = std::make_unique<SshRemoteProcessRunner>();

    connect(m_killRunner.get(), &SshRemoteProcessRunner::processClosed, this, [this, callback]() {
        QString errorMessage;
        if (m_killRunner->processExitStatus() != SshRemoteProcess::NormalExit) {
            errorMessage = m_killRunner->processErrorString();
        } else if (m_killRunner->processExitCode() != 0) {
            errorMessage = tr("The remote 'kill' command exited with %1. stderr: %2")
                .arg(m_killRunner->processExitCode())
                .arg(QString::fromUtf8(m_killRunner->readAllStandardError()));
        }
        if (!errorMessage.isEmpty()) {
            qCWarning(sfdk).noquote() << tr("Failed to send signal to the remote process: %1")
                .arg(errorMessage);
        }
        (this->*callback)(errorMessage.isEmpty());
        m_killRunner.reset(nullptr);
    });

    connect(m_killRunner.get(), &SshRemoteProcessRunner::connectionError, this, [this, callback]() {
        qCWarning(sfdk).noquote() << tr("Connection error while trying to send signal to the remote process: %1")
            .arg(m_killRunner->lastConnectionErrorString());
        (this->*callback)(false);
        m_killRunner.reset(nullptr);
    });

    QString killCommand = QString("kill -%1 -- -%2 %2").arg(signal).arg(m_processId);
    qCDebug(sfdk) << "Attempting to send signal to the remote process using" << killCommand;

    m_killRunner->run(killCommand.toUtf8(), m_sshConnectionParams);
}

QString RemoteProcess::environmentString(const QProcessEnvironment &environment)
{
    if (environment.isEmpty())
        return {};

    QStringList keys = environment.keys();
    keys.sort();

    QStringList assignments;
    for (const QString key : qAsConst(keys)) {
        const QString value = Utils::QtcProcess::quoteArgUnix(environment.value(key));
        assignments.append(key + "=" + value);
    }

    return assignments.join(' ');
}

void RemoteProcess::onProcessStarted()
{
    if (m_interactive) {
        m_stdin = std::make_unique<QFile>(this);
        if (!m_stdin->open(stdin, QIODevice::ReadOnly | QIODevice::Unbuffered)) {
            qCWarning(sfdk) << "Unable to read from standard input while interactive mode is requested";
            return;
        }
        QSocketNotifier *const notifier = new QSocketNotifier(0, QSocketNotifier::Read, m_stdin.get());
        connect(notifier, &QSocketNotifier::activated,
                this, &RemoteProcess::handleStdin);
    }
}

void RemoteProcess::onReadyReadStandardOutput()
{
    // except for the period before the processId string is received, stdout is unbuffered
    if (m_processId != 0) {
        emit standardOutput(m_runner->readAllStandardOutput());
        return;
    }

    if (!m_stdoutBuffer.append(m_runner->readAllStandardOutput()))
        return;

    QByteArray data = m_stdoutBuffer.flush(true);
    int cut = data.indexOf('\n');
    QTC_ASSERT(cut != -1, { emit standardOutput(data); return; });
    m_processId = data.left(cut).toLongLong();
    qCDebug(sfdk) << "Remote process ID:" << m_processId;
    data.remove(0, cut + 1);
    if (!data.isEmpty())
        emit standardOutput(data);
}

void RemoteProcess::onReadyReadStandardError()
{
    if (!m_stderrBuffer.append(m_runner->readAllStandardError()))
        return;

    emit standardError(m_stderrBuffer.flush());
}

void RemoteProcess::handleStdin()
{
    m_runner->writeDataToProcess(m_stdin->readLine());
}

/*!
 * \class RemoteProcess::LineBuffer
 */

bool RemoteProcess::LineBuffer::hasData() const
{
    return !m_buffer.isEmpty();
}

bool RemoteProcess::LineBuffer::append(const QByteArray &data)
{
    m_buffer.append(data);
    return data.contains('\n') || m_buffer.size() > LINE_BUFFER_MAX;
}

QByteArray RemoteProcess::LineBuffer::flush(bool all)
{
    int cut = m_buffer.lastIndexOf('\n');
    if (all || cut == -1) {
        QByteArray retv;
        m_buffer.swap(retv);
        return retv;
    } else {
        QByteArray retv = m_buffer.left(cut + 1);
        m_buffer.remove(0, cut + 1);
        return retv;
    }
}
