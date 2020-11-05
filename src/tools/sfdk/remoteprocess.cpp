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

#include "remoteprocess.h"

#include "sfdkconstants.h"
#include "sfdkglobal.h"
#include "textutils.h"

#include <ssh/sshremoteprocessrunner.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QEventLoop>
#include <QFile>
#include <QSocketNotifier>
#include <QTimer>

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
    , m_runInTerminal(isOutputConnectedToTerminal())
{
    connect(m_runner.get(), &SshRemoteProcessRunner::processStarted,
            this, &RemoteProcess::onProcessStarted);
    connect(m_runner.get(), &SshRemoteProcessRunner::readyReadStandardOutput,
            this, &RemoteProcess::onReadyReadStandardOutput);
    connect(m_runner.get(), &SshRemoteProcessRunner::readyReadStandardError,
            this, &RemoteProcess::onReadyReadStandardError);
    connect(m_runner.get(), &SshRemoteProcessRunner::connectionError,
            this, &RemoteProcess::onConnectionError);
    connect(m_runner.get(), &SshRemoteProcessRunner::processClosed,
            this, &RemoteProcess::onProcessClosed);
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

void RemoteProcess::setRunInTerminal(bool runInTerminal)
{
    m_runInTerminal = runInTerminal;
}

/*!
 * Using \c QProcess::ForwardedInputChannel enables more terminal features
 * like turning off echo when reading passwords.  At the same time passing too
 * much information about the terminal to the remote application may not be
 * desired. Consider e.g. an application animating a progress bar up to the
 * right edge of the terminal. If that progress bar is preceded by file name
 * and the file name is subject to reverse path mapping, it breaks easily as
 * the paths on host are usualy longer than the paths under the build engine.
 * For that reason is the default value \c QProcess::ManagedInputChannel.
 */
void RemoteProcess::setInputChannelMode(QProcess::InputChannelMode inputChannelMode)
{
    m_inputChannelMode = inputChannelMode;
}

void RemoteProcess::setStandardOutputLineBuffered(bool lineBuffered)
{
    m_standardOutputLineBuffered = lineBuffered;
}

void RemoteProcess::enableLogAllOutput(std::function<const QLoggingCategory &()> category, const QString &key)
{
    if (category().isDebugEnabled()) {
        auto rtrim1eol = [](const QByteArray &a) {
            return a.endsWith('\n') ? a.chopped(1) : a;
        };
        QObject::connect(this, &RemoteProcess::standardOutput, [=](const QByteArray &data) {
            for (const QByteArray &line : rtrim1eol(data).split('\n'))
                qCDebug(category).noquote().nospace() << key << "/stdout: " << line;
        });
        QObject::connect(this, &RemoteProcess::standardError, [=](const QByteArray &data) {
            for (const QByteArray &line : rtrim1eol(data).split('\n'))
                qCDebug(category).noquote().nospace() << key << "/stderr: " << line;
        });
    }
}

int RemoteProcess::exec()
{
    QEventLoop loop;
    connect(this, &RemoteProcess::finished, &loop, &QEventLoop::exit);

    start();
    const int exitCode = loop.exec();

    return exitCode;
}

void RemoteProcess::start()
{
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

    if (m_runInTerminal)
        m_runner->runInTerminal(fullCommand, m_sshConnectionParams, m_inputChannelMode);
    else
        m_runner->run(fullCommand, m_sshConnectionParams);

    started();
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
        m_runner->cancel(); // In case it was still starting
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

    m_killRunner->run(killCommand, m_sshConnectionParams);
}

void RemoteProcess::finish()
{
    QTC_ASSERT(!m_finished, return);
    m_finished = true;

    const int exitCode = m_startedOk && m_runner->processExitStatus() == SshRemoteProcess::NormalExit
        && m_runner->processExitCode() != 255 // SSH error
        ? m_runner->processExitCode()
        : SFDK_EXIT_ABNORMAL;

    if (exitCode == SFDK_EXIT_ABNORMAL)
        emit processError(m_runner->processErrorString());

    while (m_killRunner != nullptr)
        QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents);

    exited();

    emit finished(exitCode);
}

QString RemoteProcess::environmentString(const QProcessEnvironment &environment)
{
    if (environment.isEmpty())
        return {};

    QStringList keys = environment.keys();
    keys.sort();

    QStringList assignments;
    for (const QString &key : qAsConst(keys)) {
        const QString value = Utils::QtcProcess::quoteArgUnix(environment.value(key));
        assignments.append(key + "=" + value);
    }

    return assignments.join(' ');
}

void RemoteProcess::onProcessStarted()
{
    m_startedOk = true;

    qDebug() << "XXX m_runInTerminal" << m_runInTerminal <<
        "m_inputChannelMode" << m_inputChannelMode;
    if (m_runInTerminal && m_inputChannelMode == QProcess::ManagedInputChannel) {
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

void RemoteProcess::onConnectionError()
{
    emit connectionError(m_runner->lastConnectionErrorString());
    finish();
}

void RemoteProcess::onProcessClosed()
{
    finish();
}

void RemoteProcess::onReadyReadStandardOutput()
{
    if (!m_standardOutputLineBuffered && m_processId != 0) {
        emit standardOutput(m_runner->readAllStandardOutput());
        return;
    }

    if (!m_stdoutBuffer.append(m_runner->readAllStandardOutput()))
        return;

    if (m_processId != 0) {
        emit standardOutput(m_stdoutBuffer.flush());
        return;
    }

    QByteArray data = m_stdoutBuffer.flush(!m_standardOutputLineBuffered);
    int cut = data.indexOf('\n');
    QTC_ASSERT(cut != -1, { emit standardOutput(data); return; });
    // ssh with input connected to terminal translates LF to CRLF, hence the need to trim
    m_processId = data.left(cut).trimmed().toLongLong();
    qCDebug(sfdk) << "Remote process ID:" << m_processId << "all data:" << data;
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
    const auto line = m_stdin->readLine();
    qDebug() << "YYY line:" << line;
    m_runner->writeDataToProcess(line);
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
