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
#include "merremoteprocess.h"

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

int Command::executeRemoteCommand(const QString &command)
{
    // Convert to build engine paths
    QString mappedCommand = remotePathMapping(command);

    maybeUndoCMakePathMapping();

    // Execute in build engine
    MerRemoteProcess process;
    process.setSshParameters(sshParameters());
    process.setCommand(mappedCommand);
    const int rc = process.executeAndWait();

    maybeDoCMakePathMapping();

    return rc;
}

int Command::executeSfdk(const QStringList &arguments)
{
    QTextStream qerr(stderr);

    static const QString program = QCoreApplication::applicationDirPath()
        + '/' + RELATIVE_REVERSE_LIBEXEC_PATH + "/sfdk" QTC_HOST_EXE_SUFFIX;

    QStringList allArguments;
    if (!targetName().isEmpty())
        allArguments << "-c" << "target=" + targetName();
    if (!deviceName().isEmpty())
        allArguments << "-c" << "device=" + deviceName();
    allArguments << sfdkOptions();
    allArguments << arguments;

    auto environment = QProcessEnvironment::systemEnvironment();
    environment.insert(Mer::Constants::SAILFISH_SDK_FRONTEND,
            Mer::Constants::SAILFISH_SDK_FRONTEND_ID);

    qerr << "+ " << program << ' ' << QtcProcess::joinArgs(allArguments, Utils::OsTypeLinux)
        << endl;

    QProcess process;
    process.setProcessChannelMode(QProcess::ForwardedChannels);
    process.setProgram(program);
    process.setArguments(allArguments);
    process.setProcessEnvironment(environment);

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

QString Command::sharedHomePath() const
{
    return m_sharedHomePath;
}

void Command::setSharedHomePath(const QString& path)
{
    m_sharedHomePath = QDir::fromNativeSeparators(path);
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
    m_rawArgs = args;
    m_args.clear();
    foreach (const QString& arg,args)
        m_args.append(shellSafeArgument(arg));
}

QStringList Command::arguments() const
{
    return m_args;
}

QStringList Command::rawArguments() const
{
    return m_rawArgs;
}

void Command::setSshParameters(const QSsh::SshConnectionParameters& params)
{
    m_sshConnectionParams = params;
}

QSsh::SshConnectionParameters Command::sshParameters() const
{
    return m_sshConnectionParams;
}

QString Command::deviceName() const
{
    return m_deviceName;
}

void Command::setDeviceName(const QString& device)
{
    m_deviceName = device;
}

QString Command::shellSafeArgument(const QString &argument) const
{
    //unix style
    static QRegExp reg1(QLatin1String("^'(.*)'$"));
    //windows style
    static QRegExp reg2(QLatin1String("^\"(.*)\"$"));

    QString safeArgument = argument;

    //unquote for file exits
    if (reg1.indexIn(argument) != -1) {
        safeArgument = reg1.cap(1);
    }

    if (reg2.indexIn(argument) != -1) {
        safeArgument = reg2.cap(1);
    }

    if (QFile::exists(safeArgument)) {
        safeArgument = QDir::fromNativeSeparators(safeArgument);
        if (safeArgument.indexOf(QLatin1Char(' ')) > -1)
            safeArgument = QLatin1Char('\'') + safeArgument + QLatin1Char('\'');
        return safeArgument;
    }
    return argument;
}

QString Command::remotePathMapping(const QString& command) const
{
    QString result = command;

    result.prepend(QLatin1String("cd '") + QDir::currentPath()
                   + QLatin1String("' && "));

    if (!sharedTargetPath().isEmpty() && !targetName().isEmpty()) {
        result.replace(sharedTargetPath() + "/" + targetName() + "/", "/");
        result.replace(sharedTargetPath() + "/" + targetName(), "/");
    }

    // First replace shared home and then shared src (error prone!)
    if (!sharedHomePath().isEmpty()) {
      QDir homeDir(sharedHomePath());
      result.replace(homeDir.canonicalPath(), Sfdk::Constants::BUILD_ENGINE_SHARED_HOME_MOUNT_POINT);
    }
    if (!sharedSourcePath().isEmpty()) {
      QDir srcDir(sharedSourcePath());
      result.replace(srcDir.canonicalPath(), Sfdk::Constants::BUILD_ENGINE_SHARED_SRC_MOUNT_POINT,
		     (Utils::HostOsInfo::isWindowsHost()?Qt::CaseInsensitive:Qt::CaseSensitive));
    }
    result = result.trimmed();
    return result;
}

bool Command::isValid() const
{
    QDir curDir(QDir::currentPath());
    QDir homeDir(QDir::fromNativeSeparators(QDir::cleanPath(sharedHomePath())));
    QDir srcDir(QDir::fromNativeSeparators(QDir::cleanPath(sharedSourcePath())));

    QString currentPath = curDir.canonicalPath();
    QString cleanSharedHome = homeDir.canonicalPath();
    QString cleanSharedSrc = srcDir.canonicalPath();

    if (Utils::HostOsInfo::isWindowsHost()) {
        currentPath = currentPath.toLower();
        cleanSharedHome = cleanSharedHome.toLower();
        cleanSharedSrc = cleanSharedSrc.toLower();
    }

    if (!currentPath.startsWith(cleanSharedHome) && (sharedSourcePath().isEmpty() ||
                                                     !currentPath.startsWith(cleanSharedSrc))) {
        QString message = QString::fromLatin1("Project ERROR: Project is outside of shared home \"%1\" and shared src \"%2\".")
                .arg(QDir::toNativeSeparators(sharedHomePath())).arg(QDir::toNativeSeparators(sharedSourcePath()));
        fprintf(stderr, "%s", qPrintable(message));
        fflush(stderr);
        return false;
    }
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

    QDirIterator it(".", {"CMakeCache.txt", "*.cbp", "QtCreatorDeployment.txt"}, QDir::Files,
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

        data.replace(Sfdk::Constants::BUILD_ENGINE_SHARED_HOME_MOUNT_POINT, sharedHomePath());
        data.replace(Sfdk::Constants::BUILD_ENGINE_SHARED_SRC_MOUNT_POINT, sharedSourcePath());

        if (QFileInfo(path).fileName() == "CMakeCache.txt") {
            auto fixVar = [](QString *data, const QString &name, const QString &preferredType,
                    const QString &value, bool addIfMissing) {
                const QRegularExpression re(QString("%1:(FILEPATH|INTERNAL|PATH|STRING)=.*")
                        .arg(QRegularExpression::escape(name)));
                if (data->contains(re)) {
                    data->replace(re, name + ":\\1=" + value);
                } else if (addIfMissing) {
                    data->append("\n");
                    data->append("//No help, variable specified on the command line.\n");
                    data->append(name + ":" + preferredType + "=" + value + "\n");
                }
            };

            const bool usesQt = data.contains(QRegularExpression("Qt5Core_DIR:(PATH|STRING)=.*"));

            // Map paths and add variables that were filtered out by
            // CMakeCommand::filterQtcSettings(). If not added, QtC would complain
            // about changed configuration.
            fixVar(&data, "CMAKE_CXX_COMPILER", "FILEPATH", sdkToolsPath() + "/gcc", true);
            fixVar(&data, "CMAKE_C_COMPILER", "FILEPATH", sdkToolsPath() + "/gcc", true);
            fixVar(&data, "CMAKE_COMMAND", "INTERNAL", sdkToolsPath() + "/cmake", true);
            fixVar(&data, "CMAKE_SYSROOT", "PATH", sharedTargetRoot, true);
            fixVar(&data, "CMAKE_PREFIX_PATH", "PATH", sharedTargetRoot + "/usr", true);
            fixVar(&data, "QT_QMAKE_EXECUTABLE", "FILEPATH", sdkToolsPath() + "/qmake", usesQt);
        }

        data.replace("/usr/include/", sharedTargetRoot + "/usr/include/");

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
