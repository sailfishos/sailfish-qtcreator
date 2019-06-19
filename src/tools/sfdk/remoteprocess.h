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

#include <QByteArray>
#include <QObject>
#include <QProcessEnvironment>

#include <ssh/sshconnection.h>

#include "task.h"

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

    void setProgram(const QString& program);
    void setArguments(const QStringList& arguments);
    void setWorkingDirectory(const QString& workingDirectory);
    void setSshParameters(const QSsh::SshConnectionParameters& params);
    void setExtraEnvironment(const QProcessEnvironment &extraEnvironment);
    void setInteractive(bool enabled);
    int exec();

signals:
    void standardOutput(const QByteArray &data);
    void standardError(const QByteArray &data);
    void connectionError(const QString &errorMessage);

protected:
    void beginTerminate() override;
    void beginStop() override;
    void beginContinue() override;

private:
    void kill(const QString &signal, void (RemoteProcess::*callback)(bool));
    static QString environmentString(const QProcessEnvironment &environment);

private slots:
    void onProcessStarted();
    void onReadyReadStandardOutput();
    void onReadyReadStandardError();
    void handleStdin();

private:
    std::unique_ptr<QSsh::SshRemoteProcessRunner> m_runner;
    std::unique_ptr<QSsh::SshRemoteProcessRunner> m_killRunner;
    QString m_program;
    QStringList m_arguments;
    QString m_workingDirectory;
    QSsh::SshConnectionParameters m_sshConnectionParams;
    QProcessEnvironment m_extraEnvironment;
    bool m_interactive = true;
    qint64 m_processId = 0;
    QFile *m_stdin = nullptr;
    LineBuffer m_stdoutBuffer;
    LineBuffer m_stderrBuffer;
};

} // namespace Sfdk
