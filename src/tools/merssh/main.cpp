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
#include "gcccommand.h"
#include "generatekeyscommand.h"
#include "makecommand.h"
#include "qmakecommand.h"

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
            << "merssh <command> [args]..." << endl
            << "commands:" << endl
            << CommandFactory::commands().join(' ') << endl
            << "environment variables (required):" << endl
            << Sfdk::Constants::MER_SSH_SDK_TOOLS << endl;
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

void initQSsh()
{
    // See ProjectExplorerPlugin::extensionsInitialized()
    QSettings qtcSettings{QLatin1String(Core::Constants::IDE_SETTINGSVARIANT_STR),
            QLatin1String(Core::Constants::IDE_CASED_ID)};
    QSsh::SshSettings::loadSettings(&qtcSettings);
    const QString gitBinary = qtcSettings.value("Git/BinaryPath", "git")
        .toString();
    const QStringList rawGitSearchPaths = qtcSettings.value("Git/Path")
        .toString().split(':', Qt::SkipEmptyParts);
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
    // Avoid redirecting output to system logs, which happens (not only) when
    // sfdk is invoked by GUI applications on Windows.
    if (qEnvironmentVariableIsEmpty("QT_FORCE_STDERR_LOGGING"))
        qputenv("QT_FORCE_STDERR_LOGGING", "1");

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

    arguments = unquoteArguments(arguments);

    const QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();

    command->setSdkToolsPath(environment.value(QLatin1String(Sfdk::Constants::MER_SSH_SDK_TOOLS)));
    command->setArguments(arguments);

    if (!command->isValid()) {
       qCritical() << "Invalid command arguments" << endl;
       printUsage();
       return 1;
    }

    CommandInvoker invoker(command.data());
    return a.exec();
}
