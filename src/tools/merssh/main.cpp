/****************************************************************************
**
** Copyright (C) 2012 - 2018 Jolla Ltd.
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
#include "enginectlcommand.h"
#include "gcccommand.h"
#include "generatekeyscommand.h"
#include "lupdatecommand.h"
#include "makecommand.h"
#include "qmakecommand.h"
#include "rpmcommand.h"
#include "rpmvalidationcommand.h"
#include "wwwproxycommand.h"

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
            << "merssh [name=value]... <command> [args]..." << endl
            << "commands:" << endl
            << CommandFactory::commands().join(' ') << endl
            << "environment variables - project parameters:" << endl
            << Mer::Constants::MER_SSH_TARGET_NAME << endl
            << Mer::Constants::MER_SSH_SHARED_HOME << endl
            << Mer::Constants::MER_SSH_SHARED_TARGET << endl
            << Mer::Constants::MER_SSH_SHARED_SRC << endl
            << Mer::Constants::MER_SSH_SDK_TOOLS << endl
            << Mer::Constants::MER_SSH_PROJECT_PATH << endl
            << Mer::Constants::MER_SSH_DEVICE_NAME << endl
            << "evironment variables - connection parameters:" << endl
            << Mer::Constants::MER_SSH_USERNAME << endl
            << Mer::Constants::MER_SSH_PORT << endl
            << Mer::Constants::MER_SSH_PRIVATE_KEY << endl
            << "evironment variables - used by enginectl only:" << endl
            << Mer::Constants::MER_SSH_ENGINE_NAME << endl;
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
    CommandFactory::registerCommand<EngineCtlCommand>(QLatin1String("enginectl"));
    CommandFactory::registerCommand<LUpdateCommand>(QLatin1String("lupdate"));
    CommandFactory::registerCommand<WwwProxyCommand>(QLatin1String("wwwproxy"));

    QStringList arguments  = QCoreApplication::arguments();

    //remove merssh
    arguments.takeFirst();

    // Allow to pass variable on command line instead of in environment. Used by installer where
    // environment variables cannot be set.
    const QSet<QString> environmentVariables{
        QLatin1String(Mer::Constants::MER_SSH_TARGET_NAME),
        QLatin1String(Mer::Constants::MER_SSH_SHARED_HOME),
        QLatin1String(Mer::Constants::MER_SSH_SHARED_TARGET),
        QLatin1String(Mer::Constants::MER_SSH_SHARED_SRC),
        QLatin1String(Mer::Constants::MER_SSH_SDK_TOOLS),
        QLatin1String(Mer::Constants::MER_SSH_PROJECT_PATH),
        QLatin1String(Mer::Constants::MER_SSH_DEVICE_NAME),
        QLatin1String(Mer::Constants::MER_SSH_ENGINE_NAME),
        QLatin1String(Mer::Constants::MER_SSH_USERNAME),
        QLatin1String(Mer::Constants::MER_SSH_PORT),
        QLatin1String(Mer::Constants::MER_SSH_PRIVATE_KEY),
    };
    while (!arguments.isEmpty()) {
        const int equalPosition = arguments.first().indexOf('=');
        if (equalPosition == -1)
            break;
        const QString name = arguments.first().left(equalPosition);
        if (!environmentVariables.contains(name))
            break;
        const QString value = arguments.first().mid(equalPosition + 1);
        qputenv(name.toLocal8Bit(), value.toLocal8Bit());
        arguments.takeFirst();
    }

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

    if (!qobject_cast<EngineCtlCommand *>(command.data())
            && !qobject_cast<WwwProxyCommand *>(command.data()))
        arguments = unquoteArguments(arguments);

    const QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    command->setTargetName(environment.value(QLatin1String(Mer::Constants::MER_SSH_TARGET_NAME)));
    command->setSharedHomePath(environment.value(QLatin1String(Mer::Constants::MER_SSH_SHARED_HOME)));
    command->setSharedTargetPath(environment.value(QLatin1String(Mer::Constants::MER_SSH_SHARED_TARGET)));
    command->setSharedSourcePath(environment.value(QLatin1String(Mer::Constants::MER_SSH_SHARED_SRC)));
    command->setSdkToolsPath(environment.value(QLatin1String(Mer::Constants::MER_SSH_SDK_TOOLS)));
    command->setProjectPath(environment.value(QLatin1String(Mer::Constants::MER_SSH_PROJECT_PATH)));
    command->setDeviceName(environment.value(QLatin1String(Mer::Constants::MER_SSH_DEVICE_NAME)));
    command->setEngineName(environment.value(QLatin1String(Mer::Constants::MER_SSH_ENGINE_NAME)));

    QSsh::SshConnectionParameters parameters;
    parameters.setHost(QLatin1String(Mer::Constants::MER_SDK_DEFAULTHOST));
    parameters.setUserName(environment.value(QLatin1String(Mer::Constants::MER_SSH_USERNAME)));
    parameters.setPort(environment.value(QLatin1String(Mer::Constants::MER_SSH_PORT)).toInt());
    parameters.privateKeyFile = environment.value(QLatin1String(Mer::Constants::MER_SSH_PRIVATE_KEY));
    parameters.authenticationType = QSsh::SshConnectionParameters::AuthenticationTypePublicKey;
    parameters.timeout = 10;
    command->setSshParameters(parameters);
    command->setArguments(arguments);

    if (!command->isValid()) {
       qCritical() << "Invalid command arguments" << endl;
       printUsage();
       return 1;
    }

    CommandInvoker invoker(command.data());
    return a.exec();
}
