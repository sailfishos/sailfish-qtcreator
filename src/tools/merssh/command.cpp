/****************************************************************************
**
** Copyright (C) 2012-2015,2018-2019 Jolla Ltd.
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

#include "command.h"

#include <mer/merconstants.h>
#include <sfdk/sfdkconstants.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QRegularExpression>
#include <QTextStream>

namespace {
const char BACKUP_SUFFIX[] = ".qtccache";
}

using namespace Utils;

Command::Command()
{
}

Command::~Command()
{
}

int Command::executeSfdk(const QStringList &arguments)
{
    QTextStream qerr(stderr);

    static const QString program = QCoreApplication::applicationDirPath()
        + '/' + RELATIVE_REVERSE_LIBEXEC_PATH + "/sfdk" QTC_HOST_EXE_SUFFIX;

    QStringList allArguments;
    if (!targetName().isEmpty())
        allArguments << "-c" << "target=" + targetName();
    allArguments << sfdkOptions();
    allArguments << arguments;

    qerr << "+ " << program << ' ' << QtcProcess::joinArgs(allArguments, Utils::OsTypeLinux)
        << endl;

    QProcess process;
    process.setProcessChannelMode(QProcess::ForwardedChannels);
    process.setProgram(program);
    process.setArguments(allArguments);

    maybeUndoCMakePathMapping();

    int rc{};
    process.start();
    if (!process.waitForFinished(-1) || process.error() == QProcess::FailedToStart)
        rc = -2; // like QProcess::execute does
    else
        rc = process.exitStatus() == QProcess::NormalExit ? process.exitCode() : -1;

    maybeDoCMakePathMapping();

    return rc;
}

QStringList Command::sfdkOptions() const
{
    return m_sfdkOptions;
}

void Command::setSfdkOptions(const QStringList &sfdkOptions)
{
    m_sfdkOptions = sfdkOptions;
}

QString Command::targetName() const
{
    return m_targetName;
}

void Command::setTargetName(const QString& name)
{
    m_targetName = name;
}

QString Command::sharedSourcePath() const
{
    return m_sharedSourcePath;
}

void Command::setSharedSourcePath(const QString& path)
{
    m_sharedSourcePath = QDir::fromNativeSeparators(path);
}

QString Command::sharedSourceMountPoint() const
{
    return m_sharedSourceMountPoint;
}

void Command::setSharedSourceMountPoint(const QString& path)
{
    m_sharedSourceMountPoint = path;
}

QString Command::sharedTargetPath() const
{
    return m_sharedTargetPath;
}

void Command::setSharedTargetPath(const QString& path)
{
    m_sharedTargetPath = QDir::fromNativeSeparators(path);
}

QString Command::sdkToolsPath() const
{
    return m_toolsPath;
}

void Command::setSdkToolsPath(const QString& path)
{
    m_toolsPath = QDir::fromNativeSeparators(path);
}

void Command::setArguments(const QStringList &args)
{
    m_args = args;
}

QStringList Command::arguments() const
{
    return m_args;
}

bool Command::isValid() const
{
    return true;
}

QString Command::readRelativeRoot()
{
    FileReader reader;
    if (!reader.fetch("CMakeCache.txt"))
        return {};

    QString data = QString::fromUtf8(reader.data());
    QRegularExpression cmakeHomeRegexp("CMAKE_HOME_DIRECTORY:INTERNAL=(.*)");
    QRegularExpressionMatch cmakeHomeMatch = cmakeHomeRegexp.match(data);
    if (!cmakeHomeMatch.hasMatch())
        return {};

    QString cmakeHome = cmakeHomeMatch.captured(1);
    return cmakeHome + '/' + QDir(cmakeHome).relativeFilePath("/");
}

void Command::maybeDoCMakePathMapping()
{
    if (!QFile::exists("CMakeCache.txt"))
        return;

    QString sharedTargetRoot = sharedTargetPath() + "/" + targetName();
    QString relativeRoot = readRelativeRoot();
    QTC_CHECK(!relativeRoot.isEmpty());

    QDirIterator it(".", {"CMakeCache.txt", "QtCreatorDeployment.txt"}, QDir::Files,
            QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString path = it.next();
        const QString backupPath = path + BACKUP_SUFFIX;

        bool ok;

        if (QFile::exists(backupPath))
            QFile::remove(backupPath);

        ok = QFile::rename(path, backupPath);
        if (!ok) {
            fprintf(stderr, "%s", qPrintable(QString::fromLatin1("Could not back up file \"%1\"").arg(path)));
            fflush(stderr);
            continue;
        }

        FileReader reader;
        ok = reader.fetch(backupPath);
        if (!ok) {
            fprintf(stderr, "%s", qPrintable(reader.errorString()));
            fflush(stderr);
            continue;
        }

        QString data = QString::fromUtf8(reader.data());

        if (!relativeRoot.isEmpty())
            data.replace(relativeRoot, sharedTargetRoot + "/");

        data.replace(sharedSourceMountPoint(), sharedSourcePath());

        if (QFileInfo(path).fileName() == "CMakeCache.txt") {
            const bool usesQt = data.contains(QRegularExpression("Qt5Core_DIR:(PATH|STRING)=.*"));

            // Map paths and add variables that were filtered out by
            // CMakeCommand::filterQtcSettings(). If not added, QtC would complain
            // about changed configuration.
            updateOrAddToCMakeCacheIf(&data, "CMAKE_CXX_COMPILER", {"FILEPATH", "STRING"},
                    sdkToolsPath() + '/' + Sfdk::Constants::WRAPPER_GCC,
                    true);
            updateOrAddToCMakeCacheIf(&data, "CMAKE_C_COMPILER", {"FILEPATH", "STRING"},
                    sdkToolsPath() + '/' + Sfdk::Constants::WRAPPER_GCC,
                    true);
            updateOrAddToCMakeCacheIf(&data, "CMAKE_COMMAND", {"INTERNAL"},
                    sdkToolsPath() + '/' + Sfdk::Constants::WRAPPER_CMAKE,
                    false);
            updateOrAddToCMakeCacheIf(&data, "CMAKE_SYSROOT", {"PATH", "STRING"},
                    sharedTargetRoot,
                    true);
            updateOrAddToCMakeCacheIf(&data, "CMAKE_PREFIX_PATH", {"PATH", "STRING"},
                    sharedTargetRoot + "/usr",
                    true);
            // See qmakeFromCMakeCache() in cmakeprojectimporter.cpp
            updateOrAddToCMakeCacheIf(&data, "QT_QMAKE_EXECUTABLE", {"FILEPATH", "STRING"},
                    sdkToolsPath() + '/' + Sfdk::Constants::WRAPPER_QMAKE,
                    usesQt);
        }

        data.replace(QRegularExpression("((?<!INCLUDE_INSTALL_DIR:PATH=)/usr/(local/)?include\\b)"),
                sharedTargetRoot + "\\1");
        data.replace(QRegularExpression("((?<!SHARE_INSTALL_PREFIX:PATH=)/usr/share\\b)"),
                sharedTargetRoot + "\\1");

        // GCC's system include paths are under the tooling. Map them to the
        // respective directories under the target.
        data.replace(QRegularExpression("/srv/mer/toolings/[^/]+/opt/cross/"),
                sharedTargetRoot + "/opt/cross/");

        FileSaver saver(path);
        saver.write(data.toUtf8());
        ok = saver.finalize();
        if (!ok) {
            fprintf(stderr, "%s", qPrintable(saver.errorString()));
            fflush(stderr);
            continue;
        }
    }
}

void Command::maybeUndoCMakePathMapping()
{
    if (!QFile::exists(QString("CMakeCache.txt") + BACKUP_SUFFIX))
        return;

    const int suffixLength = QLatin1String(BACKUP_SUFFIX).size();
    QDirIterator it(".", {QString("*") + BACKUP_SUFFIX}, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString backupPath = it.next();
        const QString path = backupPath.chopped(suffixLength);

        bool ok;

        if (QFile::exists(path))
            QFile::remove(path);

        // Use copy instead of rename to get timestamps updated. Without that
        // QtC could continue using old data
        ok = QFile::copy(backupPath, path);
        if (!ok) {
            fprintf(stderr, "%s", qPrintable(QString::fromLatin1("Could not restore file \"%1\"").arg(path)));
            fflush(stderr);
            continue;
        }

        QFile::remove(backupPath);
    }
}

void Command::updateOrAddToCMakeCacheIf(QString *data, const QString &name, const QStringList &types,
        const QString &value, bool shouldAdd)
{
    QTC_ASSERT(!types.isEmpty(), return);
    const QRegularExpression re(QString("%1:(%2)=.*")
            .arg(name) // no need to escape, we know what we pass here
            .arg(types.join('|')));
    if (data->contains(re)) {
        data->replace(re, name + ":\\1=" + value);
    } else if (shouldAdd) {
        data->append("\n");
        data->append("//No help, variable specified on the command line.\n");
        data->append(name + ":" + types.first() + "=" + value);
        data->append("\n");
    }
}
