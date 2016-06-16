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

#include "commandfactory.h"
#include "deploycommand.h"
#include "gcccommand.h"
#include "generatekeyscommand.h"
#include "makecommand.h"
#include "qmakecommand.h"
#include "rpmcommand.h"
#include "rpmvalidationcommand.h"

#include <mer/merconstants.h>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QLoggingCategory>
#include <QProcessEnvironment>
#include <QStringList>
#include <QTimer>

void printUsage()
{
    qCritical()
            << "merssh usage:" << endl
            << "merssh <command>" << endl
            << "environment project parameters:" << endl
            << Mer::Constants::MER_SSH_TARGET_NAME << endl
            << Mer::Constants::MER_SSH_SHARED_HOME << endl
            << Mer::Constants::MER_SSH_SHARED_TARGET << endl
            << Mer::Constants::MER_SSH_SHARED_SRC << endl
            << Mer::Constants::MER_SSH_SDK_TOOLS << endl
            << Mer::Constants::MER_SSH_PROJECT_PATH << endl
            << Mer::Constants::MER_SSH_DEVICE_NAME << endl
            << "evironment connection parameters:" << endl
            << Mer::Constants::MER_SSH_USERNAME << endl
            << Mer::Constants::MER_SSH_PORT << endl
            << Mer::Constants::MER_SSH_PRIVATE_KEY << endl;
}

QStringList unquoteArguments(QStringList args) {

    QStringList result;
    //unix style
    static QRegExp reg1(QLatin1String("^'(.*)'$"));
    //windows style
    static QRegExp reg2(QLatin1String("^\"(.*)\"$"));

    foreach (const QString arg,args) {
        if (reg1.indexIn(arg) != -1){
            if (arg.indexOf(QLatin1Char(' ')) > -1)
                result.append(arg);
            else
                result.append(reg1.cap(1));
        } else if (reg2.indexIn(arg) != -1){
            if (arg.indexOf(QLatin1Char(' ')) > -1)
                result.append(arg);
            else
                result.append(reg2.cap(1));
        }  else if (arg.indexOf(QLatin1Char(' ')) == -1) {
            result.append(arg);
        } else {
            QString message = QString::fromLatin1("Unquoted argument found  \"%1\", which should be quoted. Skipping...\n").arg(arg);
            fprintf(stderr, "%s", qPrintable(message));
            fflush(stderr);
        }
    }
    return result;
}

int main(int argc, char *argv[])
{
    QLoggingCategory::setFilterRules(QLatin1String("qtc.*.debug=false"));

    QCoreApplication a(argc, argv);

    CommandFactory::registerCommand<QMakeCommand>(QLatin1String("qmake"));
    CommandFactory::registerCommand<GccCommand>(QLatin1String("gcc"));
    CommandFactory::registerCommand<MakeCommand>(QLatin1String("make"));
    CommandFactory::registerCommand<DeployCommand>(QLatin1String("deploy"));
    CommandFactory::registerCommand<RpmCommand>(QLatin1String("rpm"));
    CommandFactory::registerCommand<RpmValidationCommand>(QLatin1String("rpmvalidation"));
    CommandFactory::registerCommand<GenerateKeysCommand>(QLatin1String("generatesshkeys"));

    QStringList arguments  = QCoreApplication::arguments();

    //remove merssh
    arguments.takeFirst();

    if(arguments.isEmpty()) {
        qCritical() << "No arguments" << endl;
        printUsage();
        return 1;
    }

    QScopedPointer<Command> command(CommandFactory::create(arguments.first()));

    if(command.isNull()) {
         qCritical() << "Command not found." << endl;
         printUsage();
         return 1;
    }

    const QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    command->setTargetName(environment.value(QLatin1String(Mer::Constants::MER_SSH_TARGET_NAME)));
    command->setSharedHomePath(environment.value(QLatin1String(Mer::Constants::MER_SSH_SHARED_HOME)));
    command->setSharedTargetPath(environment.value(QLatin1String(Mer::Constants::MER_SSH_SHARED_TARGET)));
    command->setSharedSourcePath(environment.value(QLatin1String(Mer::Constants::MER_SSH_SHARED_SRC)));
    command->setSdkToolsPath(environment.value(QLatin1String(Mer::Constants::MER_SSH_SDK_TOOLS)));
    command->setProjectPath(environment.value(QLatin1String(Mer::Constants::MER_SSH_PROJECT_PATH)));
    command->setDeviceName(environment.value(QLatin1String(Mer::Constants::MER_SSH_DEVICE_NAME)));

    QSsh::SshConnectionParameters parameters;
    parameters.host = QLatin1String(Mer::Constants::MER_SDK_DEFAULTHOST);
    parameters.userName = environment.value(QLatin1String(Mer::Constants::MER_SSH_USERNAME));
    parameters.port = environment.value(QLatin1String(Mer::Constants::MER_SSH_PORT)).toInt();
    parameters.privateKeyFile = environment.value(QLatin1String(Mer::Constants::MER_SSH_PRIVATE_KEY));
    parameters.authenticationType = QSsh::SshConnectionParameters::AuthenticationTypePublicKey;
    parameters.timeout = 10;
    command->setSshParameters(parameters);
    command->setArguments(unquoteArguments(arguments));

    if (!command->isValid()) {
       qCritical() << "Invalid command arguments" << endl;
       printUsage();
       return 1;
    }

    CommandInvoker invoker(command.data());
    return a.exec();
}
