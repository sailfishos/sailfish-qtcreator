/****************************************************************************
**
** Copyright (C) 2012 - 2013 Jolla Ltd.
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

#ifndef MERSSH_H
#define MERSSH_H

#include <ssh/sshconnection.h>

#include <QObject>

QT_BEGIN_NAMESPACE
class QFile;
QT_END_NAMESPACE

namespace QSsh {
class SshRemoteProcessRunner;
} // namespace QSsh

class MerSSH : public QObject
{
    Q_OBJECT

public:
    explicit MerSSH(QObject *parent = 0);
    ~MerSSH();
    bool run(const QString &sdkToolsDir, const QString &merTargetName,
             const QString &commandType, const QString &command, bool interactive);
    static QString shellSafeArgument(const QString &argument);

protected:
    void timerEvent(QTimerEvent *event);

private slots:
    void onProcessStarted();
    void onStandardOutput();
    void onStandardError();
    void onProcessClosed(int exitStatus);
    void onConnectionError();
    void handleStdin();

private:
    void printError(const QString &message);

    int m_exitCode;
    bool m_quietOnError;
    bool m_interactive;
    QString m_merSysRoot;
    QString m_currentCacheFile;
    QByteArray m_currentCacheData;
    QSsh::SshRemoteProcessRunner *m_runner;
    QByteArray m_completeCommand;
    QSsh::SshConnectionParameters m_sshConnectionParams;
    QFile *m_stdin;
};

#endif // MERSSH_H
