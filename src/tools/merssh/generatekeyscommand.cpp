/****************************************************************************
**
** Copyright (C) 2012-2015,2019 Jolla Ltd.
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

#include "generatekeyscommand.h"

#include <ssh/sshsettings.h>

#include <QDateTime>
#include <QFile>
#include <QProcess>

GenerateKeysCommand::GenerateKeysCommand()
{
}

QString GenerateKeysCommand::name() const
{
    return QLatin1String("merssh");
}

bool GenerateKeysCommand::parseArguments()
{
    QStringList args = arguments();
    args.removeFirst();

    if (args.isEmpty() || args.first().isEmpty()) {
        qCritical() << "No private key given.";
        return false;
    } else if (args.count() > 1) {
        qCritical() << "Unexpected argument:" << args.at(1);
        return false;
    }

    m_privateKeyFileName = args.first();

    return true;
}

QString GenerateKeysCommand::unquote(const QString &arg)
{
    QString result;

    //unix style
    static QRegExp reg1(QLatin1String("^'(.*)'$"));
    //windows style
    static QRegExp reg2(QLatin1String("^\"(.*)\"$"));

    // strip quotes away
    if (reg1.indexIn(arg) != -1){
        result.append(reg1.cap(1));
    } else if (reg2.indexIn(arg) != -1){
        result.append(reg2.cap(1));
    }  else if (arg.indexOf(QLatin1Char(' ')) == -1) {
        result.append(arg);
    }
    return result;
}

int GenerateKeysCommand::execute()
{
    if (QSsh::SshSettings::keygenFilePath().isEmpty()) {
        qCritical() << "The ssh-keygen tool was not found.";
        return 1;
    }

    if(!parseArguments()) {
        qCritical() << "Could not parse arguments.";
        return 1;
    }

    fprintf(stdout, "%s", "Generating keys...");
    fflush(stdout);

    m_privateKeyFileName = unquote(m_privateKeyFileName);

    QProcess keygen;
    const QString keyComment("QtCreator/" + QDateTime::currentDateTime().toString(Qt::ISODate));
    const QStringList args{"-t", "rsa", "-b", "2048", "-N", QString(), "-C", keyComment, "-f", m_privateKeyFileName};
    QString errorMsg;
    keygen.start(QSsh::SshSettings::keygenFilePath().toString(), args);
    keygen.closeWriteChannel();

    if (!keygen.waitForStarted() || !keygen.waitForFinished())
        errorMsg = keygen.errorString();
    else if (keygen.exitStatus() != QProcess::NormalExit || keygen.exitCode() != 0)
        errorMsg = QString::fromLocal8Bit(keygen.readAllStandardError());

    if (!errorMsg.isEmpty()) {
        qCritical() << "The ssh-keygen tool at" << QSsh::SshSettings::keygenFilePath().toUserOutput()
            << "failed:" << errorMsg;
        return 1;
    }

    return 0;
}

bool GenerateKeysCommand::isValid() const
{
    return true;
}
