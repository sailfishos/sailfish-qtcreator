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

#include "task.h"
#include "textutils.h"

#include <ssh/sshconnection.h>

#include <QByteArray>
#include <QObject>
#include <QProcessEnvironment>

#include <memory>

QT_BEGIN_NAMESPACE
class QFile;
QT_END_NAMESPACE

namespace QSsh {
class SshRemoteProcessRunner;
}

namespace Sfdk {

class RemoteProcess : public Task
{
    Q_OBJECT

    class LineBuffer
    {
    public:
        bool hasData() const;
        bool append(const QByteArray &data);
        QByteArray flush(bool all = false);

    private:
        QByteArray m_buffer;
    };

public:
    explicit RemoteProcess(QObject *parent = 0);
    ~RemoteProcess() override;

    QString program() const { return m_program; }
    void setProgram(const QString& program);

    QStringList arguments() const { return m_arguments; }
    void setArguments(const QStringList& arguments);

    QString workingDirectory() const { return m_workingDirectory; }
    void setWorkingDirectory(const QString& workingDirectory);

    QSsh::SshConnectionParameters sshParameters() const { return m_sshConnectionParams; }
    void setSshParameters(const QSsh::SshConnectionParameters& params);

    QProcessEnvironment extraEnvironment() const { return m_extraEnvironment; }
    void setExtraEnvironment(const QProcessEnvironment &extraEnvironment);

    bool isRunInTerminalSet() const { return m_runInTerminal; }
    void setRunInTerminal(bool runInTerminal);

    bool isStandardOutputLineBufferedSet() const { return m_standardOutputLineBuffered; }
    void setStandardOutputLineBuffered(bool lineBuffered);

    void enableLogAllOutput(std::function<const QLoggingCategory &()> category, const QString &key);

    void start();
    int exec();

signals:
    void standardOutput(const QByteArray &data);
    void standardError(const QByteArray &data);
    void connectionError(const QString &errorMessage);
    void processError(const QString &errorMessage);
    void finished(int exitCode);

protected:
    void beginTerminate() override;
    void beginStop() override;
    void beginContinue() override;

private:
    void kill(const QString &signal, void (RemoteProcess::*callback)(bool));
    void finish();
    static QString environmentString(const QProcessEnvironment &environment);

private slots:
    void onProcessStarted();
    void onConnectionError();
    void onProcessClosed();
    void onReadyReadStandardOutput();
    void onReadyReadStandardError();

private:
    std::unique_ptr<QSsh::SshRemoteProcessRunner> m_runner;
    std::unique_ptr<QSsh::SshRemoteProcessRunner> m_killRunner;
    QString m_program;
    QStringList m_arguments;
    QString m_workingDirectory;
    QSsh::SshConnectionParameters m_sshConnectionParams;
    QProcessEnvironment m_extraEnvironment;
    bool m_runInTerminal = isOutputConnectedToTerminal();
    bool m_standardOutputLineBuffered = false;
    bool m_startedOk = false;
    bool m_finished = false;
    qint64 m_processId = 0;
    std::unique_ptr<QFile> m_stdin;
    LineBuffer m_stdoutBuffer;
    LineBuffer m_stderrBuffer;
};

} // namespace Sfdk
