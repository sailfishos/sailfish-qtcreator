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

#include <QCoreApplication>
#include <QFile>
#include <QStringList>
#include <QDebug>

#include "merssh.h"
#include "mer/merconstants.h"
#include <ssh/sshkeygenerator.h>

void printUsage()
{
    qCritical()
            << "merssh usage:" << endl
            << "merssh <options> <command>" << endl
            << "  " << Mer::Constants::MERSSH_PARAMETER_SDKTOOLSDIR
            << "  directory with Qt Creator's target helpers" << endl
            << "  " << Mer::Constants::MERSSH_PARAMETER_MERTARGET
            << "       the mer target name" << endl
            << "  " << Mer::Constants::MERSSH_PARAMETER_COMMANDTYPE
            << "     ['standard'|'sb2']" << endl
            << "All options are mandatory." << endl;
}

bool generateSshKeys(const QString &privateKeyFileName, const QString &publicKeyFileName)
{
    QSsh::SshKeyGenerator keyGen;
    if (!keyGen.generateKeys(QSsh::SshKeyGenerator::Rsa,
                             QSsh::SshKeyGenerator::Mixed, 2048,
                             QSsh::SshKeyGenerator::DoNotOfferEncryption)) {
        qCritical() << "Cannot generate the ssh keys." << endl;
        return false;
    }

    QFile privateKeyFile(privateKeyFileName);
    if (!privateKeyFile.open(QIODevice::WriteOnly)) {
        qCritical() << "Cannot write file:" << privateKeyFileName << endl;
        return false;
    }
    privateKeyFile.write(keyGen.privateKey());
    if (!QFile::setPermissions(privateKeyFileName, QFile::ReadOwner | QFile::WriteOwner)) {
        qCritical() << "Cannot set permissions of file:" << privateKeyFileName << endl;
        return false;
    }

    QFile publicKeyFile(publicKeyFileName);
    if (!publicKeyFile.open(QIODevice::WriteOnly)) {
        qCritical() << "Cannot write file:" << publicKeyFileName << endl;
        return false;
    }
    publicKeyFile.write(keyGen.publicKey());

    return true;
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QString sdkToolsDir;
    QString merTarget;
    QString commandType;
    QString command;
    QStringList commandPieces;
    QString *currentParameter = 0;

    QStringList arguments = QCoreApplication::arguments();
    arguments.takeFirst();

    if (arguments.length() == 3 && arguments.first() == QLatin1String("generatesshkeys"))
        return generateSshKeys(arguments.at(1), arguments.at(2)) ? 0 : 1;

    foreach (QString argument, arguments) {
        argument = argument.trimmed();
        if (argument == QLatin1String(Mer::Constants::MERSSH_PARAMETER_SDKTOOLSDIR)) {
            currentParameter = &sdkToolsDir;
            commandPieces.clear();
        } else if (argument == QLatin1String(Mer::Constants::MERSSH_PARAMETER_MERTARGET)) {
            currentParameter = &merTarget;
            commandPieces.clear();
        } else if (argument == QLatin1String(Mer::Constants::MERSSH_PARAMETER_COMMANDTYPE)) {
            currentParameter = &commandType;
            commandPieces.clear();
        } else {
            if (currentParameter) {
                *currentParameter = argument;
                currentParameter = 0;
            } else {
                commandPieces.append(MerSSH::shellSafeArgument(argument));
            }
        }
    }
    command = commandPieces.join(QLatin1String(" "));

    if (sdkToolsDir.isEmpty() || merTarget.isEmpty()
            || commandType.isEmpty() || command.isEmpty())  {
        printUsage();
        return 1;
    }

    MerSSH merSSH(&a);
    const bool parametersCorrect =
            merSSH.run(sdkToolsDir + QLatin1Char('/'), merTarget, commandType, command);
    if (!parametersCorrect)
        return 1;

    return a.exec();
}
