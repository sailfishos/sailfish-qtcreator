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

#include "command.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDirIterator>
#include <QFile>
#include <QSettings>
#include <QTextCodec>
#include <QTimer>
#include <QTranslator>

#include <utils/fileutils.h>

#include "commandlineparser.h"
#include "configuration.h"
#include "dispatch.h"
#include "sdkmanager.h"
#include "session.h"
#include "sfdkconstants.h"
#include "sfdkglobal.h"
#include "task.h"
#include "textutils.h"

using namespace Sfdk;

namespace {
const char MODULES_PATH[] = "sfdk/modules";
const char VERSION_FILE_PATH[] = "sdk-release";
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    qSetMessagePattern(
            "%{if-category}%{category}: %{endif}"
            "["
            "%{if-debug}D%{endif}"
            "%{if-info}I%{endif}"
            "%{if-warning}W%{endif}"
            "%{if-critical}C%{endif}"
            "%{if-fatal}F%{endif}"
            "] "
            "%{message}");

#ifdef Q_OS_WIN
    // This makes utf-8 output work out of box under mintty. Under native
    // Windows console one needs to ensure an unicode capable font is used (e.g.
    // Lucida) and set the codepage using `chcp 65001`.
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
#endif

#ifdef Q_OS_WIN
    {
        // See Qt Creator's main
        QDir rootDir = app.applicationDirPath();
        rootDir.cdUp();
        QString mySettingsPath = QDir::toNativeSeparators(rootDir.canonicalPath());
        mySettingsPath += QDir::separator() + QLatin1String("settings");
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, mySettingsPath);
    }
#endif

    TaskManager taskManager;

    const QString resourcePath = QDir::cleanPath(app.applicationDirPath() + '/'
            + RELATIVE_DATA_PATH);

    // TODO Qt Creator uses a more advanced approach, maybe worth to follow parts of it
    const QString translationsPath = QDir::cleanPath(resourcePath + "/translations");
    QTranslator translator;
    if (translator.load(QLocale(), "qtcreator", "_", translationsPath))
        app.installTranslator(&translator);
    QTranslator qtTranslator;
    if (qtTranslator.load(QLocale(), "qt", "_", translationsPath))
        app.installTranslator(&qtTranslator);

    Configuration configuration;

    Dispatcher dispatcher;
    dispatcher.registerWorkerType<BuiltinWorker>("builtin");
    dispatcher.registerWorkerType<EngineWorker>("engine-command");

    const QDir modulesDir(resourcePath + '/' + MODULES_PATH);
    const QFileInfoList modules = modulesDir.entryInfoList({"*.json"}, QDir::Files, QDir::Name);
    if (modules.isEmpty()) {
        qCCritical(sfdk) << "No module found. Check your installation.";
        return EXIT_FAILURE;
    }
    for (const QFileInfo &module : modules) {
        if (!dispatcher.load(module.filePath()))
            return EXIT_FAILURE;
    }
    if (Dispatcher::domain(Constants::GENERAL_DOMAIN_NAME) == nullptr) {
        qCCritical(sfdk) << "No module in the" << QLatin1String(Constants::GENERAL_DOMAIN_NAME)
            << "domain. Check your installation.";
        return EXIT_FAILURE;
    }

    Session session;

    if (!configuration.load())
        return EXIT_FAILURE;

    bool showVersion = false;

    CommandLineParser parser(app.arguments());
    switch (parser.result()) {
    case CommandLineParser::BadUsage:
        return EXIT_FAILURE;

    case CommandLineParser::Dispatch:
        break;

    case CommandLineParser::Usage:
        return EXIT_SUCCESS;

    case CommandLineParser::Version:
        showVersion = true;
    }

    SdkManager sdkManager;
    if (!sdkManager.isValid())
        return EXIT_FAILURE;

    if (showVersion) {
        QString versionFile = SdkManager::installationPath() + '/' + VERSION_FILE_PATH;
        Utils::FileReader reader;
        QString errorString;
        if (!reader.fetch(versionFile, &errorString)) {
            qCCritical(sfdk) << "Error reading version file" << versionFile << ":"
                << qPrintable(errorString);
            return EXIT_FAILURE;
        }

        qout() << reader.data();
        return EXIT_SUCCESS;
    }

    /*
     * Setting verbosity so late in order to not complicate command line parsing
     * has the following (not so) bad implications:
     *
     * 1) Debug messages originated prior to this point are not enabled when
     *    they should be
     *    - These can be still enabled using other method like exporting
     *      QT_LOGGING_RULES
     *    - This can be actually considered as an advantace - it allows to
     *      divide the debug messages to two groups: initialization related and
     *      command execution related, former of which are less likely to be
     *      needed while debugging usual issues at user side
     *
     * 2) Info messages originated prior to this point are not supressed when
     *    they should be
     *    - There should be no need to issue info messages prior to this point.
     *      Check logging conventions.
     */
    switch (parser.verbosity()) {
    case CommandLineParser::Quiet:
        QLoggingCategory::setFilterRules("sfdk.info=false");
        break;
    case CommandLineParser::Normal:
        /* noop */
        break;
    case CommandLineParser::Debug:
        QLoggingCategory::setFilterRules("sfdk.debug=true");
        SdkManager::setEnableReversePathMapping(false);
        break;
    }

    QTimer::singleShot(0, [&]() {
        const Worker *const worker = parser.command()->module->worker;
        int exitCode;
        switch (worker->run(parser.command(), parser.commandArguments(), &exitCode)) {
        case Worker::NoSuchCommand:
            app.exit(EXIT_FAILURE);
            return;

        case Worker::BadUsage:
            parser.commandBriefUsage(qerr(), parser.command());
            app.exit(EXIT_FAILURE);
            return;

        case Worker::CrashExit:
            app.exit(EXIT_FAILURE);
            return;

        case Worker::NormalExit:
            app.exit(exitCode);
            return;
        }

        Q_ASSERT(false);
        app.exit(EXIT_FAILURE);
    });

    return app.exec();
}
