/****************************************************************************
**
** Copyright (C) 2012 - 2014 Jolla Ltd.
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

#include "merremoteprocess.h"

#include <mer/merconstants.h>
#include <ssh/sshremoteprocessrunner.h>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QProcessEnvironment>
#include <QSettings>
#include <QSocketNotifier>

MerRemoteProcess::MerRemoteProcess(QObject *parent)
    : QSsh::SshRemoteProcessRunner(parent)
    , m_cache(false)
    , m_interactive(true)
{
    connect(this, &MerRemoteProcess::processStarted,
            this, &MerRemoteProcess::onProcessStarted);
    connect(this, &MerRemoteProcess::readyReadStandardOutput,
            this, &MerRemoteProcess::onStandardOutput);
    connect(this, &MerRemoteProcess::readyReadStandardError,
            this, &MerRemoteProcess::onStandardError);
    connect(this, &MerRemoteProcess::connectionError,
            this, &MerRemoteProcess::onConnectionError);
}

MerRemoteProcess::~MerRemoteProcess()
{

}

void MerRemoteProcess::setCommand(const QString& command)
{
    m_command = command;
}

void MerRemoteProcess::setSshParameters(const QSsh::SshConnectionParameters& params)
{
    m_sshConnectionParams = params;
}

void MerRemoteProcess::setIntercative(bool enabled)
{
    m_interactive = enabled;
}

int MerRemoteProcess::executeAndWait()
{
    QEventLoop loop;
    connect(this, &MerRemoteProcess::connectionError,
            &loop, &QEventLoop::quit);
    connect(this, &MerRemoteProcess::processClosed,
            &loop, &QEventLoop::quit);
    QSsh::SshRemoteProcessRunner::run(m_command.toUtf8(), m_sshConnectionParams);
    loop.exec();
    if(processExitStatus() == QSsh::SshRemoteProcess::NormalExit)
        return processExitCode();
    else
        return 1;
}

void MerRemoteProcess::onProcessStarted()
{
    if (m_interactive) {
        m_stdin = new QFile();
        if (!m_stdin->open(stdin, QIODevice::ReadOnly | QIODevice::Unbuffered)) {
            qDebug("Warning: Cannot read from standard input.");
            return;
        }
        QSocketNotifier * const notifier = new QSocketNotifier(0, QSocketNotifier::Read, this);
        connect(notifier, &QSocketNotifier::activated,
                this, &MerRemoteProcess::handleStdin);
    }
}

void MerRemoteProcess::onStandardOutput()
{
    if (!m_cache) {
        const QByteArray output(readAllStandardOutput());
        fprintf(stdout, "%s", output.constData());
        fflush(stdout);
    }
}

void MerRemoteProcess::onStandardError()
{
    if (!m_cache) {
        const QByteArray output(readAllStandardError());
        fprintf(stderr, "%s", output.constData());
        fflush(stderr);
    }
}

void MerRemoteProcess::onConnectionError()
{
    if (!m_cache) {
        fprintf(stderr, "%s",
                qPrintable(QString::fromLatin1(
                               "Project ERROR: Could not connect to MerSDK Virtual Machine. %1\n")
                                         .arg(lastConnectionErrorString())));
        fflush(stderr);
    }
}

void MerRemoteProcess::handleStdin()
{
    writeDataToProcess(m_stdin->readLine());
}


