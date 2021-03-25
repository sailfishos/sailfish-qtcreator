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

    QStringList filteredArguments = arguments();
    if (filteredArguments.at(1) != "--build") {
        // sfdk-cmake requires path to source directory to appear as the very first argument
        // and that the current working dirrectory is the build directory (so -B is redundant,
        // and so better dropped for greater clarity)
        QTC_ASSERT(filteredArguments.count() >= 5, return 1);
        QTC_ASSERT(filteredArguments.at(1) == "-S", return 1);
        QTC_ASSERT(filteredArguments.at(3) == "-B", return 1);
        QTC_ASSERT(filteredArguments.at(4) == QDir::currentPath(), return 1);
        filteredArguments.removeAt(4);
        filteredArguments.removeAt(3);
        filteredArguments.removeAt(1);
        filteredArguments.insert(2, "--"); // no doubts
    }

    return executeSfdk(filteredArguments);
}

bool CMakeCommand::isValid() const
{
    return Command::isValid() && !targetName().isEmpty() && !sdkToolsPath().isEmpty();
}
