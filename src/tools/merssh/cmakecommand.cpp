/****************************************************************************
**
** Copyright (C) 2020 Open Mobile Platform LLC.
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

#include "cmakecommand.h"

#include <mer/merconstants.h>
#include <sfdk/sfdkconstants.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QRegularExpression>

using namespace Utils;

CMakeCommand::CMakeCommand()
{

}

QString CMakeCommand::name() const
{
    return QLatin1String("cmake");
}

int CMakeCommand::execute()
{
    if (arguments().contains(QLatin1String("-E"))
            && arguments().contains(QLatin1String("capabilities"))) {
        m_cacheFile = QLatin1String(Sfdk::Constants::CMAKE_CAPABILITIES_CACHE);
    } else if (arguments().contains(QLatin1String("--version"))) {
        m_cacheFile = QLatin1String(Sfdk::Constants::CMAKE_VERSION_CACHE);
    } else if (arguments().contains(QLatin1String("--help"))) {
        QTC_ASSERT(false, return 1);
    }

    if (!m_cacheFile.isEmpty()) {
        m_cacheFile.prepend(sdkToolsPath() + QDir::separator());

        if (QFile::exists(m_cacheFile)) {
            QFile cacheFile(m_cacheFile);
            if (!cacheFile.open(QIODevice::ReadOnly)) {
                fprintf(stderr, "%s",qPrintable(QString::fromLatin1("Cannot read cached file \"%1\"").arg(m_cacheFile)));
                fflush(stderr);
                return 1;
            }
            fprintf(stdout, "%s", cacheFile.readAll().constData());
            fflush(stdout);
            return 0;
        }

        return 1;
    }

    QTC_ASSERT(arguments().count() > 1, return 1);
    QTC_ASSERT(arguments().first() == "cmake", return 1);

    QStringList filteredArguments;
    for (const QString &argument : arguments()) {
        // See CMakeProjectManager::BuildDirManager::writeConfigurationIntoBuildDirectory()
        if (argument.endsWith('/' + QLatin1String("qtcsettings.cmake"))) {
            const QString filteredPath = filterQtcSettings(argument);
            if (filteredPath.isNull())
                return 1;
            filteredArguments.append(filteredPath);
            continue;
        }

        filteredArguments.append(argument);
    }

    if (filteredArguments.at(1) != "--build") {
        // sfdk-cmake requires path to source directory to appear as the very first argument
        bool lastArgLooksLikePathToSource = QFile::exists(filteredArguments.last() + "/CMakeLists.txt")
            || QDir(filteredArguments.last() + "/rpm").exists();
        QTC_ASSERT(lastArgLooksLikePathToSource, return 1);
        filteredArguments.insert(1, "--");
        filteredArguments.insert(1, filteredArguments.takeLast());
    }

    return executeSfdk(filteredArguments);
}

bool CMakeCommand::isValid() const
{
    return Command::isValid() && !targetName().isEmpty() && !sdkToolsPath().isEmpty();
}

QString CMakeCommand::filterQtcSettings(const QString &path)
{
    const QFileInfo info(path);
    const QString filteredPath = info.path() + QDir::separator() + info.baseName() + "-filtered."
        + info.completeSuffix();

    if (!QFile::exists(path)) {
        fprintf(stderr, "%s", qPrintable(QString::fromLatin1("File does not exist \"%1\"").arg(path)));
        fflush(stderr);
        return {};
    }

    bool ok;

    FileReader reader;
    ok = reader.fetch(path);
    if (!ok) {
        fprintf(stderr, "%s", qPrintable(reader.errorString()));
        fflush(stderr);
        return {};
    }

    QString data = QString::fromUtf8(reader.data());

    auto removeVariable = [](QString *data, const QString &name) {
        const QString re = QString(R"(^set\("%1".*\n?)")
            .arg(QRegularExpression::escape(name));
        data->remove(QRegularExpression(re, QRegularExpression::MultilineOption));
    };

    // See MerSdkManager::ensureCmakeToolIsSet() and Command::maybeDoCMakePathMapping()
    removeVariable(&data, "CMAKE_CXX_COMPILER");
    removeVariable(&data, "CMAKE_C_COMPILER");
    removeVariable(&data, "CMAKE_PREFIX_PATH");
    removeVariable(&data, "CMAKE_SYSROOT");
    removeVariable(&data, "QT_QMAKE_EXECUTABLE");

    FileSaver saver(filteredPath);
    saver.write(data.toUtf8());
    ok = saver.finalize();
    if (!ok) {
        fprintf(stderr, "%s", qPrintable(saver.errorString()));
        fflush(stderr);
        return {};
    }

    return filteredPath;
}
