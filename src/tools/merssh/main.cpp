/****************************************************************************
**
** Copyright (C) 2012-2016,2018-2019 Jolla Ltd.
** Copyright (C) 2019-2020 Open Mobile Platform LLC.
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
#include "cmakecommand.h"
#include "deploycommand.h"
#include "gcccommand.h"
#include "generatekeyscommand.h"
#include "lupdatecommand.h"
#include "makecommand.h"
#include "makeinstallcommand.h"
#include "qmakecommand.h"
#include "rpmcommand.h"
#include "rpmvalidationcommand.h"

#include <app/app_version.h>
#include <mer/merconstants.h>
#include <sfdk/sfdkconstants.h>
#include <ssh/sshsettings.h>
#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QLoggingCategory>
#include <QProcessEnvironment>
#include <QSettings>
#include <QStringList>
#include <QTimer>

using namespace Utils;

void printUsage()
{
    qCritical()
            << "merssh usage:" << endl
            << "merssh [name=value]... <command> [args]..." << endl
            << "commands:" << endl
            << CommandFactory::commands().join(' ') << endl
            << "environment variables - project parameters:" << endl
            << Sfdk::Constants::MER_SSH_TARGET_NAME << endl
            << Sfdk::Constants::MER_SSH_SHARED_TARGET << endl
            << Sfdk::Constants::MER_SSH_SHARED_SRC << endl
            << Sfdk::Constants::MER_SSH_SHARED_SRC_MOUNT_POINT << endl
            << Sfdk::Constants::MER_SSH_SDK_TOOLS << endl
            << Sfdk::Constants::MER_SSH_DEVICE_NAME << endl;
}

QStringList unquoteArguments(const QStringList &arguments)
{
    const bool abortOnMeta = false;
    QtcProcess::SplitError splitError;
    const QStringList result = QtcProcess::splitArgs(arguments.join(QLatin1Char(' ')),
            OsTypeLinux, abortOnMeta, &splitError);
    QTC_ASSERT(splitError == QtcProcess::SplitOk, return {});

    return result;
}

QStringList argumentsFromEnvironment(const QProcessEnvironment &environment, const QString &name, bool *ok)
{
    *ok = false;

    const QString value = environment.value(name);
    const bool abortOnMeta = true;
    QtcProcess::SplitError error;
    const QStringList arguments = QtcProcess::splitArgs(value, Utils::OsTypeLinux,
            abortOnMeta, &error);
    if (error != QtcProcess::SplitOk) {
        qCritical() << "Failed to parse content of" << name << "environment variable" << endl;
        return {};
    }

    *ok = true;
    return arguments;
}

void initQSsh()
{
    // See ProjectExplorerPlugin::extensionsInitialized()
    QSettings qtcSettings{QLatin1String(Core::Constants::IDE_SETTINGSVARIANT_STR),
            QLatin1String(Core::Constants::IDE_CASED_ID)};
    QSsh::SshSettings::loadSettings(&qtcSettings);
    const QString gitBinary = qtcSettings.value("Git/BinaryPath", "git")
        .toString();
    const QStringList rawGitSearchPaths = qtcSettings.value("Git/Path")
        .toString().split(':', QString::SkipEmptyParts);
    const auto searchPathRetriever = [=] {
        Utils::FilePaths searchPaths;
        searchPaths << Utils::FilePath::fromString(QCoreApplication::applicationDirPath());
        if (Utils::HostOsInfo::isWindowsHost()) {
            const Utils::FilePaths gitSearchPaths = Utils::transform(rawGitSearchPaths,
                    [](const QString &rawPath) { return Utils::FilePath::fromString(rawPath); });
            const Utils::FilePath fullGitPath = Utils::Environment::systemEnvironment()
                    .searchInPath(gitBinary, gitSearchPaths);
            if (!fullGitPath.isEmpty()) {
                searchPaths << fullGitPath.parentDir()
                            << fullGitPath.parentDir().parentDir() + "/usr/bin";
            }
        }
        return searchPaths;
    };
    QSsh::SshSettings::setExtraSearchPathRetriever(searchPathRetriever);
}

int main(int argc, char *argv[])
{
    QLoggingCategory::setFilterRules(QLatin1String("qtc.*.debug=false"));

    QCoreApplication a(argc, argv);

#ifdef Q_OS_WIN
    {
        // See Qt Creator's main
        QDir rootDir = a.applicationDirPath();
        rootDir.cdUp();
        QString mySettingsPath = QDir::toNativeSeparators(rootDir.canonicalPath());
        mySettingsPath += QDir::separator() + QLatin1String("settings");
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, mySettingsPath);
    }
#endif

    // Needed by generatesshkeys to locate ssh-keygen
    initQSsh();

    CommandFactory::registerCommand<QMakeCommand>(QLatin1String("qmake"));
    CommandFactory::registerCommand<CMakeCommand>(QLatin1String("cmake"));
    CommandFactory::registerCommand<GccCommand>(QLatin1String("gcc"));
    CommandFactory::registerCommand<MakeCommand>(QLatin1String("make"));
    CommandFactory::registerCommand<MakeInstallCommand>(QLatin1String("make-install"));
    CommandFactory::registerCommand<DeployCommand>(QLatin1String("deploy"));
    CommandFactory::registerCommand<RpmCommand>(QLatin1String("rpm"));
    CommandFactory::registerCommand<RpmValidationCommand>(QLatin1String("rpmvalidation"));
    CommandFactory::registerCommand<GenerateKeysCommand>(QLatin1String("generatesshkeys"));
    CommandFactory::registerCommand<LUpdateCommand>(QLatin1String("lupdate"));

    QStringList arguments  = QCoreApplication::arguments();

    //remove merssh
    arguments.takeFirst();

    // Allow to pass variable on command line instead of in environment. Used by installer where
    // environment variables cannot be set.
    const QSet<QString> environmentVariables{
        QLatin1String(Sfdk::Constants::MER_SSH_TARGET_NAME),
        QLatin1String(Sfdk::Constants::MER_SSH_SHARED_TARGET),
        QLatin1String(Sfdk::Constants::MER_SSH_SHARED_SRC),
        QLatin1String(Sfdk::Constants::MER_SSH_SHARED_SRC_MOUNT_POINT),
        QLatin1String(Sfdk::Constants::MER_SSH_SDK_TOOLS),
        QLatin1String(Sfdk::Constants::MER_SSH_DEVICE_NAME),
        QLatin1String(Sfdk::Constants::MER_SSH_SFDK_OPTIONS),
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

    qputenv("SFDK_NO_SESSION", "1");

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

    arguments = unquoteArguments(arguments);

    const QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();

    bool ok;
    const QStringList sfdkOptions = argumentsFromEnvironment(environment,
            Sfdk::Constants::MER_SSH_SFDK_OPTIONS, &ok);
    if (!ok)
        return 1;
    command->setSfdkOptions(sfdkOptions);

    command->setTargetName(environment.value(QLatin1String(Sfdk::Constants::MER_SSH_TARGET_NAME)));
    command->setSharedTargetPath(environment.value(QLatin1String(Sfdk::Constants::MER_SSH_SHARED_TARGET)));
    command->setSharedSourcePath(environment.value(QLatin1String(Sfdk::Constants::MER_SSH_SHARED_SRC)));
    command->setSharedSourceMountPoint(environment.value(QLatin1String(Sfdk::Constants::MER_SSH_SHARED_SRC_MOUNT_POINT)));
    command->setSdkToolsPath(environment.value(QLatin1String(Sfdk::Constants::MER_SSH_SDK_TOOLS)));
    command->setDeviceName(environment.value(QLatin1String(Sfdk::Constants::MER_SSH_DEVICE_NAME)));
    command->setArguments(arguments);

    if (!command->isValid()) {
       qCritical() << "Invalid command arguments" << endl;
       printUsage();
       return 1;
    }

    CommandInvoker invoker(command.data());
    return a.exec();
}
