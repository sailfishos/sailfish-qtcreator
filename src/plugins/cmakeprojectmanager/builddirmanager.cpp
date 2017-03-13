/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "builddirmanager.h"
#include "cmakebuildconfiguration.h"
#include "cmakekitinformation.h"
#include "cmakeparser.h"
#include "cmakeprojectmanager.h"
#include "cmaketool.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/target.h>
#include <projectexplorer/taskhub.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/synchronousprocess.h>

#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QRegularExpression>
#include <QSet>
#include <QTemporaryDir>

// --------------------------------------------------------------------
// Helper:
// --------------------------------------------------------------------

namespace CMakeProjectManager {
namespace Internal {

static QStringList toArguments(const CMakeConfig &config, const ProjectExplorer::Kit *k) {
    return Utils::transform(config, [k](const CMakeConfigItem &i) -> QString {
        QString a = QString::fromLatin1("-D");
        a.append(QString::fromUtf8(i.key));
        switch (i.type) {
        case CMakeConfigItem::FILEPATH:
            a.append(QLatin1String(":FILEPATH="));
            break;
        case CMakeConfigItem::PATH:
            a.append(QLatin1String(":PATH="));
            break;
        case CMakeConfigItem::BOOL:
            a.append(QLatin1String(":BOOL="));
            break;
        case CMakeConfigItem::STRING:
            a.append(QLatin1String(":STRING="));
            break;
        case CMakeConfigItem::INTERNAL:
            a.append(QLatin1String(":INTERNAL="));
            break;
        case CMakeConfigItem::STATIC:
            a.append(QLatin1String(":STATIC="));
            break;
        }
        a.append(i.expandedValue(k));

        return a;
    });
}

// --------------------------------------------------------------------
// BuildDirManager:
// --------------------------------------------------------------------

BuildDirManager::BuildDirManager(CMakeBuildConfiguration *bc) :
    m_buildConfiguration(bc)
{
    QTC_ASSERT(bc, return);
    m_projectName = sourceDirectory().fileName();

    m_reparseTimer.setSingleShot(true);
    m_reparseTimer.setInterval(2000);
    connect(&m_reparseTimer, &QTimer::timeout, this, &BuildDirManager::parse);
}

BuildDirManager::~BuildDirManager()
{
    stopProcess();
    resetData();
    delete m_tempDir;
}

const ProjectExplorer::Kit *BuildDirManager::kit() const
{
    return m_buildConfiguration->target()->kit();
}

const Utils::FileName BuildDirManager::buildDirectory() const
{
    return m_buildConfiguration->buildDirectory();
}

const Utils::FileName BuildDirManager::workDirectory() const
{
    const Utils::FileName bdir = buildDirectory();
    if (bdir.exists())
        return bdir;
    if (m_tempDir)
        return Utils::FileName::fromString(m_tempDir->path());
    return bdir;
}

const Utils::FileName BuildDirManager::sourceDirectory() const
{
    return m_buildConfiguration->target()->project()->projectDirectory();
}

const CMakeConfig BuildDirManager::intendedConfiguration() const
{
    return m_buildConfiguration->cmakeConfiguration();
}

bool BuildDirManager::isParsing() const
{
    if (m_cmakeProcess)
        return m_cmakeProcess->state() != QProcess::NotRunning;
    return false;
}

void BuildDirManager::cmakeFilesChanged()
{
    if (isParsing())
        return;

    const CMakeTool *tool = CMakeKitInformation::cmakeTool(m_buildConfiguration->target()->kit());
    if (!tool->isAutoRun())
        return;

    m_reparseTimer.start();
}

void BuildDirManager::forceReparse()
{
    if (m_buildConfiguration->target()->activeBuildConfiguration() != m_buildConfiguration)
        return;

    stopProcess();

    CMakeTool *tool = CMakeKitInformation::cmakeTool(kit());
    const QString generator = CMakeGeneratorKitInformation::generator(kit());

    QTC_ASSERT(tool, return);
    QTC_ASSERT(!generator.isEmpty(), return);

    startCMake(tool, generator, intendedConfiguration());
}

void BuildDirManager::resetData()
{
    m_hasData = false;

    m_cmakeCache.clear();
    m_projectName.clear();
    m_buildTargets.clear();
    qDeleteAll(m_files);
    m_files.clear();
}

bool BuildDirManager::persistCMakeState()
{
    if (!m_tempDir)
        return false;

    QDir dir(buildDirectory().toString());
    dir.mkpath(buildDirectory().toString());

    delete m_tempDir;
    m_tempDir = nullptr;

    resetData();
    QTimer::singleShot(0, this, &BuildDirManager::parse); // make sure signals only happen afterwards!
    return true;
}

void BuildDirManager::parse()
{
    checkConfiguration();

    CMakeTool *tool = CMakeKitInformation::cmakeTool(kit());
    const QString generator = CMakeGeneratorKitInformation::generator(kit());

    QTC_ASSERT(tool, return);
    QTC_ASSERT(!generator.isEmpty(), return);

    const QString cbpFile = CMakeManager::findCbpFile(QDir(workDirectory().toString()));
    const QFileInfo cbpFileFi = cbpFile.isEmpty() ? QFileInfo() : QFileInfo(cbpFile);
    if (!cbpFileFi.exists()) {
        // Initial create:
        startCMake(tool, generator, intendedConfiguration());
        return;
    }

    const bool mustUpdate = m_cmakeFiles.isEmpty()
            || Utils::anyOf(m_cmakeFiles, [&cbpFileFi](const Utils::FileName &f) {
                   return f.toFileInfo().lastModified() > cbpFileFi.lastModified();
               });
    if (mustUpdate) {
        startCMake(tool, generator, CMakeConfig());
    } else {
        extractData();
        m_hasData = true;
        emit dataAvailable();
    }
}

void BuildDirManager::clearCache()
{
    auto cmakeCache = Utils::FileName(workDirectory()).appendPath(QLatin1String("CMakeCache.txt"));
    auto cmakeFiles = Utils::FileName(workDirectory()).appendPath(QLatin1String("CMakeFiles"));

    const bool mustCleanUp = cmakeCache.exists() || cmakeFiles.exists();
    if (!mustCleanUp)
        return;

    Utils::FileUtils::removeRecursively(cmakeCache);
    Utils::FileUtils::removeRecursively(cmakeFiles);

    forceReparse();
}

QString BuildDirManager::projectName() const
{
    return m_projectName;
}

QList<CMakeBuildTarget> BuildDirManager::buildTargets() const
{
    return m_buildTargets;
}

QList<ProjectExplorer::FileNode *> BuildDirManager::files()
{
    return m_files;
}

QSet<Utils::FileName> BuildDirManager::cmakeFiles()
{
    return m_cmakeFiles;
}

void BuildDirManager::clearFiles()
{
    m_files.clear();
}

CMakeConfig BuildDirManager::parsedConfiguration() const
{
    if (m_cmakeCache.isEmpty()) {
        Utils::FileName cacheFile = workDirectory();
        cacheFile.appendPath(QLatin1String("CMakeCache.txt"));
        if (!cacheFile.exists())
            return m_cmakeCache;
        QString errorMessage;
        m_cmakeCache = parseConfiguration(cacheFile, &errorMessage);
        if (!errorMessage.isEmpty())
            emit errorOccured(errorMessage);
        const Utils::FileName sourceOfBuildDir
                = Utils::FileName::fromUtf8(CMakeConfigItem::valueOf("CMAKE_HOME_DIRECTORY", m_cmakeCache));
        const Utils::FileName canonicalSourceOfBuildDir = Utils::FileUtils::canonicalPath(sourceOfBuildDir);
        const Utils::FileName canonicalSourceDirectory = Utils::FileUtils::canonicalPath(sourceDirectory());
        if (canonicalSourceOfBuildDir != canonicalSourceDirectory) // Uses case-insensitive compare where appropriate
            emit errorOccured(tr("The build directory is not for %1 but for %2")
                    .arg(canonicalSourceOfBuildDir.toUserOutput(),
                         canonicalSourceDirectory.toUserOutput()));
    }
    return m_cmakeCache;
}

void BuildDirManager::stopProcess()
{
    if (!m_cmakeProcess)
        return;

    m_cmakeProcess->disconnect();

    if (m_cmakeProcess->state() == QProcess::Running) {
        m_cmakeProcess->terminate();
        if (!m_cmakeProcess->waitForFinished(500) && m_cmakeProcess->state() == QProcess::Running)
            m_cmakeProcess->kill();
    }

    cleanUpProcess();

    if (!m_future)
      return;
    m_future->reportCanceled();
    m_future->reportFinished();
    delete m_future;
    m_future = nullptr;
}

void BuildDirManager::cleanUpProcess()
{
    if (!m_cmakeProcess)
        return;

    QTC_ASSERT(m_cmakeProcess->state() == QProcess::NotRunning, return);

    m_cmakeProcess->disconnect();

    if (m_cmakeProcess->state() == QProcess::Running) {
        m_cmakeProcess->terminate();
        if (!m_cmakeProcess->waitForFinished(500) && m_cmakeProcess->state() == QProcess::Running)
            m_cmakeProcess->kill();
    }
    m_cmakeProcess->waitForFinished();
    delete m_cmakeProcess;
    m_cmakeProcess = nullptr;

    // Delete issue parser:
    m_parser->flush();
    delete m_parser;
    m_parser = nullptr;
}

void BuildDirManager::extractData()
{
    const Utils::FileName topCMake
            = Utils::FileName::fromString(sourceDirectory().toString() + QLatin1String("/CMakeLists.txt"));

    resetData();

    m_projectName = sourceDirectory().fileName();
    m_files.append(new ProjectExplorer::FileNode(topCMake, ProjectExplorer::ProjectFileType, false));
    // Do not insert topCMake into m_cmakeFiles: The project already watches that!

    // Find cbp file
    QString cbpFile = CMakeManager::findCbpFile(workDirectory().toString());
    if (cbpFile.isEmpty())
        return;
    m_cmakeFiles.insert(Utils::FileName::fromString(cbpFile));

    // Add CMakeCache.txt file:
    Utils::FileName cacheFile = workDirectory();
    cacheFile.appendPath(QLatin1String("CMakeCache.txt"));
    if (cacheFile.toFileInfo().exists())
        m_cmakeFiles.insert(cacheFile);

    // setFolderName
    CMakeCbpParser cbpparser;
    // Parsing
    if (!cbpparser.parseCbpFile(kit(), cbpFile, sourceDirectory().toString()))
        return;

    m_projectName = cbpparser.projectName();

    m_files = cbpparser.fileList();
    if (cbpparser.hasCMakeFiles()) {
        m_files.append(cbpparser.cmakeFileList());
        foreach (const ProjectExplorer::FileNode *node, cbpparser.cmakeFileList())
            m_cmakeFiles.insert(node->filePath());
    }

    // Make sure the top cmakelists.txt file is always listed:
    if (!Utils::contains(m_files, [topCMake](ProjectExplorer::FileNode *fn) { return fn->filePath() == topCMake; })) {
        m_files.append(new ProjectExplorer::FileNode(topCMake, ProjectExplorer::ProjectFileType, false));
    }

    m_buildTargets = cbpparser.buildTargets();
}

void BuildDirManager::startCMake(CMakeTool *tool, const QString &generator,
                                 const CMakeConfig &config)
{
    QTC_ASSERT(tool && tool->isValid(), return);

    QTC_ASSERT(!m_cmakeProcess, return);
    QTC_ASSERT(!m_parser, return);
    QTC_ASSERT(!m_future, return);

    // Find a directory to set up into:
    if (!buildDirectory().exists()) {
        if (!m_tempDir)
            m_tempDir = new QTemporaryDir(QDir::tempPath() + QLatin1String("/qtc-cmake-XXXXXX"));
        QTC_ASSERT(m_tempDir->isValid(), return);
    }

    // Make sure work directory exists:
    QTC_ASSERT(workDirectory().exists(), return);

    m_parser = new CMakeParser;
    QDir source = QDir(sourceDirectory().toString());
    connect(m_parser, &ProjectExplorer::IOutputParser::addTask, m_parser,
            [source](const ProjectExplorer::Task &task) {
                if (task.file.isEmpty() || task.file.toFileInfo().isAbsolute()) {
                    ProjectExplorer::TaskHub::addTask(task);
                } else {
                    ProjectExplorer::Task t = task;
                    t.file = Utils::FileName::fromString(source.absoluteFilePath(task.file.toString()));
                    ProjectExplorer::TaskHub::addTask(t);
                }
            });

    // Always use the sourceDir: If we are triggered because the build directory is getting deleted
    // then we are racing against CMakeCache.txt also getting deleted.
    const QString srcDir = sourceDirectory().toString();

    m_cmakeProcess = new Utils::QtcProcess(this);
    m_cmakeProcess->setWorkingDirectory(workDirectory().toString());
    m_cmakeProcess->setEnvironment(m_buildConfiguration->environment());

    connect(m_cmakeProcess, &QProcess::readyReadStandardOutput,
            this, &BuildDirManager::processCMakeOutput);
    connect(m_cmakeProcess, &QProcess::readyReadStandardError,
            this, &BuildDirManager::processCMakeError);
    connect(m_cmakeProcess, static_cast<void(QProcess::*)(int,  QProcess::ExitStatus)>(&QProcess::finished),
            this, &BuildDirManager::cmakeFinished);

    QString args;
    Utils::QtcProcess::addArg(&args, srcDir);
    if (!generator.isEmpty())
        Utils::QtcProcess::addArg(&args, QString::fromLatin1("-G%1").arg(generator));
    Utils::QtcProcess::addArgs(&args, toArguments(config, kit()));

    ProjectExplorer::TaskHub::clearTasks(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM);

    Core::MessageManager::write(tr("Running \"%1 %2\" in %3.")
                                .arg(tool->cmakeExecutable().toUserOutput())
                                .arg(args)
                                .arg(workDirectory().toUserOutput()));

    m_future = new QFutureInterface<void>();
    m_future->setProgressRange(0, 1);
    Core::ProgressManager::addTask(m_future->future(),
                                   tr("Configuring \"%1\"").arg(m_buildConfiguration->target()->project()->displayName()),
                                   "CMake.Configure");

    m_cmakeProcess->setCommand(tool->cmakeExecutable().toString(), args);
    m_cmakeProcess->start();
    emit configurationStarted();
}

void BuildDirManager::cmakeFinished(int code, QProcess::ExitStatus status)
{
    QTC_ASSERT(m_cmakeProcess, return);

    // process rest of the output:
    processCMakeOutput();
    processCMakeError();

    cleanUpProcess();

    extractData(); // try even if cmake failed...

    QString msg;
    if (status != QProcess::NormalExit)
        msg = tr("*** cmake process crashed!");
    else if (code != 0)
        msg = tr("*** cmake process exited with exit code %1.").arg(code);

    if (!msg.isEmpty()) {
        Core::MessageManager::write(msg);
        ProjectExplorer::TaskHub::addTask(ProjectExplorer::Task::Error, msg,
                                          ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM);
        m_future->reportCanceled();
    } else {
        m_future->setProgressValue(1);
    }

    m_future->reportFinished();
    delete m_future;
    m_future = nullptr;

    m_hasData = true;
    emit dataAvailable();
}

static QString lineSplit(const QString &rest, const QByteArray &array, std::function<void(const QString &)> f)
{
    QString tmp = rest + Utils::SynchronousProcess::normalizeNewlines(QString::fromLocal8Bit(array));
    int start = 0;
    int end = tmp.indexOf(QLatin1Char('\n'), start);
    while (end >= 0) {
        f(tmp.mid(start, end - start));
        start = end + 1;
        end = tmp.indexOf(QLatin1Char('\n'), start);
    }
    return tmp.mid(start);
}

void BuildDirManager::processCMakeOutput()
{
    static QString rest;
    rest = lineSplit(rest, m_cmakeProcess->readAllStandardOutput(), [this](const QString &s) { Core::MessageManager::write(s); });
}

void BuildDirManager::processCMakeError()
{
    static QString rest;
    rest = lineSplit(rest, m_cmakeProcess->readAllStandardError(), [this](const QString &s) {
        m_parser->stdError(s);
        Core::MessageManager::write(s);
    });
}

void BuildDirManager::checkConfiguration()
{
    if (m_tempDir) // always throw away changes in the tmpdir!
        return;

    ProjectExplorer::Kit *k = m_buildConfiguration->target()->kit();
    const CMakeConfig cache = parsedConfiguration();
    if (cache.isEmpty())
        return; // No cache file yet.

    CMakeConfig newConfig;
    QSet<QString> changedKeys;
    QSet<QString> removedKeys;
    foreach (const CMakeConfigItem &iBc, intendedConfiguration()) {
        const CMakeConfigItem &iCache
                = Utils::findOrDefault(cache, [&iBc](const CMakeConfigItem &i) { return i.key == iBc.key; });
        if (iCache.isNull()) {
            removedKeys << QString::fromUtf8(iBc.key);
        } else if (QString::fromUtf8(iCache.value) != iBc.expandedValue(k)) {
            changedKeys << QString::fromUtf8(iBc.key);
            newConfig.append(iCache);
        } else {
            newConfig.append(iBc);
        }
    }

    if (!changedKeys.isEmpty() || !removedKeys.isEmpty()) {
        QSet<QString> total = removedKeys + changedKeys;
        QStringList keyList = total.toList();
        Utils::sort(keyList);
        QString table = QLatin1String("<table>");
        foreach (const QString &k, keyList) {
            QString change;
            if (removedKeys.contains(k))
                change = tr("<removed>");
            else
                change = QString::fromUtf8(CMakeConfigItem::valueOf(k.toUtf8(), cache)).trimmed();
            if (change.isEmpty())
                change = tr("<empty>");
            table += QString::fromLatin1("\n<tr><td>%1</td><td>%2</td></tr>").arg(k).arg(change.toHtmlEscaped());
        }
        table += QLatin1String("\n</table>");

        QPointer<QMessageBox> box = new QMessageBox(Core::ICore::mainWindow());
        box->setText(tr("CMake configuration has changed on disk."));
        box->setInformativeText(tr("The CMakeCache.txt file has changed: %1").arg(table));
        box->setStandardButtons(QMessageBox::Discard | QMessageBox::Apply);
        box->setDefaultButton(QMessageBox::Discard);

        int ret = box->exec();
        if (ret == QMessageBox::Apply)
            m_buildConfiguration->setCMakeConfiguration(newConfig);
    }
}

static QByteArray trimCMakeCacheLine(const QByteArray &in) {
    int start = 0;
    while (start < in.count() && (in.at(start) == ' ' || in.at(start) == '\t'))
        ++start;

    return in.mid(start, in.count() - start - 1);
}

static QByteArrayList splitCMakeCacheLine(const QByteArray &line) {
    const int colonPos = line.indexOf(':');
    if (colonPos < 0)
        return QByteArrayList();

    const int equalPos = line.indexOf('=', colonPos + 1);
    if (equalPos < colonPos)
        return QByteArrayList();

    return QByteArrayList() << line.mid(0, colonPos)
                            << line.mid(colonPos + 1, equalPos - colonPos - 1)
                            << line.mid(equalPos + 1);
}

static CMakeConfigItem::Type fromByteArray(const QByteArray &type) {
    if (type == "BOOL")
        return CMakeConfigItem::BOOL;
    if (type == "STRING")
        return CMakeConfigItem::STRING;
    if (type == "FILEPATH")
        return CMakeConfigItem::FILEPATH;
    if (type == "PATH")
        return CMakeConfigItem::PATH;
    QTC_CHECK(type == "INTERNAL" || type == "STATIC");

    return CMakeConfigItem::INTERNAL;
}

CMakeConfig BuildDirManager::parseConfiguration(const Utils::FileName &cacheFile,
                                                QString *errorMessage)
{
    CMakeConfig result;
    QFile cache(cacheFile.toString());
    if (!cache.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (errorMessage)
            *errorMessage = tr("Failed to open %1 for reading.").arg(cacheFile.toUserOutput());
        return CMakeConfig();
    }

    QSet<QByteArray> advancedSet;
    QByteArray documentation;
    while (!cache.atEnd()) {
        const QByteArray line = trimCMakeCacheLine(cache.readLine());

        if (line.isEmpty() || line.startsWith('#'))
            continue;

        if (line.startsWith("//")) {
            documentation = line.mid(2);
            continue;
        }

        const QByteArrayList pieces = splitCMakeCacheLine(line);
        if (pieces.isEmpty())
            continue;

        QTC_ASSERT(pieces.count() == 3, continue);
        const QByteArray key = pieces.at(0);
        const QByteArray type = pieces.at(1);
        const QByteArray value = pieces.at(2);

        if (key.endsWith("-ADVANCED") && value == "1") {
            advancedSet.insert(key.left(key.count() - 9 /* "-ADVANCED" */));
        } else {
            CMakeConfigItem::Type t = fromByteArray(type);
            result << CMakeConfigItem(key, t, documentation, value);
        }
    }

    // Set advanced flags:
    for (int i = 0; i < result.count(); ++i) {
        CMakeConfigItem &item = result[i];
        item.isAdvanced = advancedSet.contains(item.key);
    }

    Utils::sort(result, CMakeConfigItem::sortOperator());

    return result;
}

void BuildDirManager::maybeForceReparse()
{
    checkConfiguration();

    const QByteArray GENERATOR_KEY = "CMAKE_GENERATOR";
    const QByteArray EXTRA_GENERATOR_KEY = "CMAKE_EXTRA_GENERATOR";
    const QByteArray CMAKE_COMMAND_KEY = "CMAKE_COMMAND";

    const QByteArrayList criticalKeys
            = QByteArrayList() << GENERATOR_KEY << CMAKE_COMMAND_KEY;

    if (!m_hasData) {
        forceReparse();
        return;
    }

    const CMakeConfig currentConfig = parsedConfiguration();

    const CMakeTool *tool = CMakeKitInformation::cmakeTool(kit());
    QTC_ASSERT(tool, return); // No cmake... we should not have ended up here in the first place
    const QString kitGenerator = CMakeGeneratorKitInformation::generator(kit());
    int pos = kitGenerator.lastIndexOf(QLatin1String(" - "));
    const QString extraKitGenerator = (pos > 0) ? kitGenerator.left(pos) : QString();
    const QString mainKitGenerator = (pos > 0) ? kitGenerator.mid(pos + 3) : kitGenerator;
    CMakeConfig targetConfig = m_buildConfiguration->cmakeConfiguration();
    targetConfig.append(CMakeConfigItem(GENERATOR_KEY, CMakeConfigItem::INTERNAL,
                                        QByteArray(), mainKitGenerator.toUtf8()));
    if (!extraKitGenerator.isEmpty())
        targetConfig.append(CMakeConfigItem(EXTRA_GENERATOR_KEY, CMakeConfigItem::INTERNAL,
                                            QByteArray(), extraKitGenerator.toUtf8()));
    targetConfig.append(CMakeConfigItem(CMAKE_COMMAND_KEY, CMakeConfigItem::INTERNAL,
                                        QByteArray(), tool->cmakeExecutable().toUserOutput().toUtf8()));
    Utils::sort(targetConfig, CMakeConfigItem::sortOperator());

    bool mustReparse = false;
    auto ccit = currentConfig.constBegin();
    auto kcit = targetConfig.constBegin();

    while (ccit != currentConfig.constEnd() && kcit != targetConfig.constEnd()) {
        if (ccit->key == kcit->key) {
            if (ccit->value != kcit->value) {
                if (criticalKeys.contains(kcit->key)) {
                        clearCache();
                        return;
                }
                mustReparse = true;
            }
            ++ccit;
            ++kcit;
        } else {
            if (ccit->key < kcit->key) {
                ++ccit;
            } else {
                ++kcit;
                mustReparse = true;
            }
        }
    }

    // If we have keys that do not exist yet, then reparse.
    //
    // The critical keys *must* be set in cmake configuration, so those were already
    // handled above.
    if (mustReparse || kcit != targetConfig.constEnd())
        forceReparse();
}

} // namespace Internal
} // namespace CMakeProjectManager
