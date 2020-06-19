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

#include "qmljsmodelmanager.h"
#include "qmljstoolsconstants.h"
#include "qmljssemanticinfo.h"
#include "qmljsbundleprovider.h"

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <cpptools/cppmodelmanager.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>
#include <qmljs/qmljsbind.h>
#include <qmljs/qmljsfindexportedcpptypes.h>
#include <qmljs/qmljsplugindumper.h>
#include <qtsupport/qmldumptool.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtsupportconstants.h>
#include <texteditor/textdocument.h>

#include <utils/algorithm.h>
#include <utils/hostosinfo.h>
#include <utils/mimetypes/mimedatabase.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <utils/runextensions.h>
#include <QTextDocument>
#include <QTextStream>
#include <QTimer>
#include <QRegExp>
#include <QSet>
#include <QString>
#include <QLibraryInfo>
#include <qglobal.h>

using namespace Utils;
using namespace Core;
using namespace ProjectExplorer;
using namespace QmlJS;

namespace QmlJSTools {
namespace Internal {

static void setupProjectInfoQmlBundles(ModelManagerInterface::ProjectInfo &projectInfo)
{
    Target *activeTarget = nullptr;
    if (projectInfo.project)
        activeTarget = projectInfo.project->activeTarget();
    Kit *activeKit = activeTarget ? activeTarget->kit() : KitManager::defaultKit();
    const QHash<QString, QString> replacements = {{QLatin1String("$(QT_INSTALL_QML)"), projectInfo.qtQmlPath}};

    for (IBundleProvider *bp : IBundleProvider::allBundleProviders())
        bp->mergeBundlesForKit(activeKit, projectInfo.activeBundle, replacements);

    projectInfo.extendedBundle = projectInfo.activeBundle;

    if (projectInfo.project) {
        QSet<Kit *> currentKits;
        foreach (const Target *t, projectInfo.project->targets())
            currentKits.insert(t->kit());
        currentKits.remove(activeKit);
        foreach (Kit *kit, currentKits) {
            for (IBundleProvider *bp : IBundleProvider::allBundleProviders())
                bp->mergeBundlesForKit(kit, projectInfo.extendedBundle, replacements);
        }
    }
}

ModelManagerInterface::ProjectInfo ModelManager::defaultProjectInfoForProject(
        Project *project) const
{
    ModelManagerInterface::ProjectInfo projectInfo;
    projectInfo.project = project;
    projectInfo.qmlDumpEnvironment = Utils::Environment::systemEnvironment();
    Target *activeTarget = nullptr;
    if (project) {
        const QSet<QString> qmlTypeNames = { Constants::QML_MIMETYPE ,Constants::QBS_MIMETYPE,
                                             Constants::QMLPROJECT_MIMETYPE,
                                             Constants::QMLTYPES_MIMETYPE,
                                             Constants::QMLUI_MIMETYPE };
        projectInfo.sourceFiles = Utils::transform(project->files([&qmlTypeNames](const Node *n) {
            if (!Project::SourceFiles(n))
                return false;
            const FileNode *fn = n->asFileNode();
            return fn && fn->fileType() == FileType::QML
                    && qmlTypeNames.contains(Utils::mimeTypeForFile(fn->filePath().toString(),
                                                                    MimeMatchMode::MatchExtension).name());
        }), &FilePath::toString);
        activeTarget = project->activeTarget();
    }
    Kit *activeKit = activeTarget ? activeTarget->kit() : KitManager::defaultKit();
    QtSupport::BaseQtVersion *qtVersion = QtSupport::QtKitAspect::qtVersion(activeKit);

    bool preferDebugDump = false;
    bool setPreferDump = false;
    projectInfo.tryQmlDump = false;

    if (activeTarget) {
        if (BuildConfiguration *bc = activeTarget->activeBuildConfiguration()) {
            preferDebugDump = bc->buildType() == BuildConfiguration::Debug;
            setPreferDump = true;
            // Append QML2_IMPORT_PATH if it is defined in build configuration.
            // It enables qmlplugindump to correctly dump custom plugins or other dependent
            // plugins that are not installed in default Qt qml installation directory.
            projectInfo.qmlDumpEnvironment.appendOrSet("QML2_IMPORT_PATH", bc->environment().expandedValueForKey("QML2_IMPORT_PATH"), ":");
        }

        const auto appTargets = activeTarget->applicationTargets();
        for (const auto &target : appTargets) {
            if (target.targetFilePath.isEmpty())
                continue;
            projectInfo.applicationDirectories.append(target.targetFilePath.parentDir().toString());
        }
    }
    if (!setPreferDump && qtVersion)
        preferDebugDump = (qtVersion->defaultBuildConfig() & QtSupport::BaseQtVersion::DebugBuild);
    if (qtVersion && qtVersion->isValid()) {
        projectInfo.tryQmlDump = project && qtVersion->type() == QLatin1String(QtSupport::Constants::DESKTOPQT);
        projectInfo.qtQmlPath = qtVersion->qmlPath().toFileInfo().canonicalFilePath();
        projectInfo.qtVersionString = qtVersion->qtVersionString();
    } else {
        projectInfo.qtQmlPath = QFileInfo(QLibraryInfo::location(QLibraryInfo::Qml2ImportsPath)).canonicalFilePath();
        projectInfo.qtVersionString = QLatin1String(qVersion());
    }

    if (projectInfo.tryQmlDump) {
        QtSupport::QmlDumpTool::pathAndEnvironment(activeKit,
                                                   preferDebugDump, &projectInfo.qmlDumpPath,
                                                   &projectInfo.qmlDumpEnvironment);
        projectInfo.qmlDumpHasRelocatableFlag = qtVersion->hasQmlDumpWithRelocatableFlag();
    } else {
        projectInfo.qmlDumpPath.clear();
        projectInfo.qmlDumpEnvironment.clear();
        projectInfo.qmlDumpHasRelocatableFlag = true;
    }
    setupProjectInfoQmlBundles(projectInfo);
    return projectInfo;
}

QHash<QString,Dialect> ModelManager::initLanguageForSuffix() const
{
    QHash<QString,Dialect> res = ModelManagerInterface::languageForSuffix();

    if (ICore::instance()) {
        MimeType jsSourceTy = Utils::mimeTypeForName(Constants::JS_MIMETYPE);
        foreach (const QString &suffix, jsSourceTy.suffixes())
            res[suffix] = Dialect::JavaScript;
        MimeType qmlSourceTy = Utils::mimeTypeForName(Constants::QML_MIMETYPE);
        foreach (const QString &suffix, qmlSourceTy.suffixes())
            res[suffix] = Dialect::Qml;
        MimeType qbsSourceTy = Utils::mimeTypeForName(Constants::QBS_MIMETYPE);
        foreach (const QString &suffix, qbsSourceTy.suffixes())
            res[suffix] = Dialect::QmlQbs;
        MimeType qmlProjectSourceTy = Utils::mimeTypeForName(Constants::QMLPROJECT_MIMETYPE);
        foreach (const QString &suffix, qmlProjectSourceTy.suffixes())
            res[suffix] = Dialect::QmlProject;
        MimeType qmlUiSourceTy = Utils::mimeTypeForName(Constants::QMLUI_MIMETYPE);
        foreach (const QString &suffix, qmlUiSourceTy.suffixes())
            res[suffix] = Dialect::QmlQtQuick2Ui;
        MimeType jsonSourceTy = Utils::mimeTypeForName(Constants::JSON_MIMETYPE);
        foreach (const QString &suffix, jsonSourceTy.suffixes())
            res[suffix] = Dialect::Json;
    }
    return res;
}

QHash<QString,Dialect> ModelManager::languageForSuffix() const
{
    static QHash<QString,Dialect> res = initLanguageForSuffix();
    return res;
}

ModelManager::ModelManager()
{
    qRegisterMetaType<QmlJSTools::SemanticInfo>("QmlJSTools::SemanticInfo");
    loadDefaultQmlTypeDescriptions();
}

ModelManager::~ModelManager() = default;

void ModelManager::delayedInitialization()
{
    CppTools::CppModelManager *cppModelManager = CppTools::CppModelManager::instance();
    // It's important to have a direct connection here so we can prevent
    // the source and AST of the cpp document being cleaned away.
    connect(cppModelManager, &CppTools::CppModelManager::documentUpdated,
            this, &ModelManagerInterface::maybeQueueCppQmlTypeUpdate, Qt::DirectConnection);

    connect(SessionManager::instance(), &SessionManager::projectRemoved,
            this, &ModelManager::removeProjectInfo);
    connect(SessionManager::instance(), &SessionManager::startupProjectChanged,
            this, &ModelManager::updateDefaultProjectInfo);

    ViewerContext qbsVContext;
    qbsVContext.language = Dialect::QmlQbs;
    qbsVContext.paths.append(ICore::resourcePath() + QLatin1String("/qbs"));
    setDefaultVContext(qbsVContext);
}

void ModelManager::loadDefaultQmlTypeDescriptions()
{
    if (ICore::instance()) {
        loadQmlTypeDescriptionsInternal(ICore::resourcePath());
        loadQmlTypeDescriptionsInternal(ICore::userResourcePath());
    }
}

void ModelManager::writeMessageInternal(const QString &msg) const
{
    MessageManager::write(msg, MessageManager::Flash);
}

ModelManagerInterface::WorkingCopy ModelManager::workingCopyInternal() const
{
    WorkingCopy workingCopy;

    if (!Core::ICore::instance())
        return workingCopy;

    foreach (IDocument *document, DocumentModel::openedDocuments()) {
        const QString key = document->filePath().toString();
        if (auto textDocument = qobject_cast<const TextEditor::TextDocument *>(document)) {
            // TODO the language should be a property on the document, not the editor
            if (DocumentModel::editorsForDocument(document).constFirst()
                    ->context().contains(ProjectExplorer::Constants::QMLJS_LANGUAGE_ID)) {
                workingCopy.insert(key, textDocument->plainText(),
                                   textDocument->document()->revision());
            }
        }
    }

    return workingCopy;
}

void ModelManager::updateDefaultProjectInfo()
{
    // needs to be performed in the ui thread
    Project *currentProject = SessionManager::startupProject();
    setDefaultProject(containsProject(currentProject)
                            ? projectInfo(currentProject)
                            : defaultProjectInfoForProject(currentProject),
                      currentProject);
}


void ModelManager::addTaskInternal(const QFuture<void> &result, const QString &msg,
                                   const char *taskId) const
{
    ProgressManager::addTask(result, msg, taskId);
}

} // namespace Internal
} // namespace QmlJSTools
