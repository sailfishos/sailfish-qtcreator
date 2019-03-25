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

#ifndef REOMOTE_PROCESS_H
#define REOMOTE_PROCESS_H

#include <ssh/sshconnection.h>
#include <ssh/sshremoteprocessrunner.h>

#include <QEventLoop>
#include <QObject>

QT_BEGIN_NAMESPACE
class QFile;
QT_END_NAMESPACE

class MerRemoteProcess : public QSsh::SshRemoteProcessRunner
{
    Q_OBJECT
public:
    explicit MerRemoteProcess(QObject *parent = 0);
    ~MerRemoteProcess() override;
    int executeAndWait();
    void setIntercative(bool enabled);
    void setSshParameters(const QSsh::SshConnectionParameters& params);
    void setCommand(const QString& command);

private:
    static QString forwardEnvironment(const QString &command);

private slots:
    void onProcessStarted();
    void onStandardOutput();
    void onStandardError();
    void onConnectionError();
    void handleStdin();

private:
    bool m_interactive;
    QString m_command;
    QSsh::SshConnectionParameters m_sshConnectionParams;
    QFile *m_stdin;
};

#endif // REOMOTE_PROCESS_H
