/****************************************************************************
**
** Copyright (C) 2012 - 2013 Jolla Ltd.
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

#include "merspecupdater.h"
#include "mersdkmanager.h"
#include "merspecifykitinformation.h"
#include "mersdkkitinformation.h"

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>

#include <qt4projectmanager/qt4project.h>
#include <qt4projectmanager/qt4nodes.h>

#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/progressmanager/futureprogress.h>

#include <qtsupport/qtkitinformation.h>

#include <utils/fileutils.h>
#include <utils/environment.h>
#include <utils/synchronousprocess.h>

using namespace ProjectExplorer;
using namespace QtSupport;
using namespace Qt4ProjectManager;

const char SECTION_START[] = "# This section is overwritten by Qt Creator. Do not remove the comments.";
const char SECTION_END[] = "# Sections ends.";

namespace Mer {
namespace Internal {

MerSpecUpdater::MerSpecUpdater(QObject *parent) :
    QObject(parent)
{
    connect(MerSdkManager::instance(), SIGNAL(initialized()), SLOT(init()));
    connect(&m_watcher, SIGNAL(fileChanged(QString)), SLOT(onYamlFileChanged(QString)));
}

void MerSpecUpdater::init()
{
    // This is called just once after the MerSdkManager is initialized.
    const SessionManager *sessionManager = ProjectExplorerPlugin::instance()->session();
    const QList<Project *> projects = sessionManager->projects();
    // O(n2)
    foreach (Project *project, projects) {
        connect(project, SIGNAL(addedTarget(ProjectExplorer::Target*)),
                SLOT(onTargetAddedToProject(ProjectExplorer::Target*)));
        connect(project, SIGNAL(removedTarget(ProjectExplorer::Target*)),
                SLOT(onTargetRemovedFromProject(ProjectExplorer::Target*)));
        onProjectAdded(project);
    }

    connect(sessionManager, SIGNAL(projectAdded(ProjectExplorer::Project*)),
            SLOT(onProjectAdded(ProjectExplorer::Project*)));
    connect(sessionManager, SIGNAL(projectRemoved(ProjectExplorer::Project*)),
            SLOT(onProjectRemoved(ProjectExplorer::Project*)));
}

void MerSpecUpdater::monitorYamlFile(const Project *proj)
{
    const Qt4Project *project = proj ? qobject_cast<const Qt4Project *>(proj)
                                     : qobject_cast<const Qt4Project *>(sender());
    if (!project || !m_projectsToMonitor.contains(project))
        return;

    // Get the yaml file. It should usually be <project_name>.yaml
    const QString expectedYamlFile = QString::fromLatin1("%1.yaml").arg(project->displayName());
    const QStringList allFiles = project->files(Project::ExcludeGeneratedFiles);
    QString yamlFile;
    foreach (const QString &file, allFiles) {
        if (file.endsWith(expectedYamlFile)) {
            yamlFile = file;
            break;
        }
    }
    // Try to find the yaml in /rpm folder
    if (yamlFile.isEmpty()) {
        QDir projDir(project->projectDirectory());
        QStringList rpm = projDir.entryList(QStringList() << QLatin1String("rpm"), QDir::Dirs);
        if (rpm.count()) {
            QDir rpmDir(rpm.first());
            QStringList files = rpmDir.entryList(QStringList() << expectedYamlFile, QDir::Files);
            if (files.count())
                yamlFile = files.first();
        }
    }
    QStringList currentFiles = m_projectFileMap.values(project);
    if (currentFiles.count())
        m_watcher.removePaths(currentFiles);
    if (!yamlFile.isEmpty()) {
        m_watcher.addPath(yamlFile);
        m_projectFileMap.insert(project, yamlFile);
        onYamlFileChanged(yamlFile);
    }
}

void MerSpecUpdater::onTargetAddedToProject(Target *target)
{
    const Project *project = target->project();
    if (MerSdkManager::instance()->merKits().contains(target->kit())
            && !m_projectsToMonitor.contains(project)) {
        monitorProject(project);
    }
}

void MerSpecUpdater::onTargetRemovedFromProject(Target *target)
{
    Project *project = target->project();
    const QList<Kit *> merKits = MerSdkManager::instance()->merKits();
    if (merKits.contains(target->kit()) && m_projectsToMonitor.contains(project)) {
        bool monitor = false;
        foreach (Kit *kit, merKits) {
            monitor = project->supportsKit(kit);
            if (monitor)
                break;
        }
        if (!monitor) {
            m_projectsToMonitor.removeOne(project);
            project->disconnect(this);
            QStringList currentFiles = m_projectFileMap.values(project);
            if (currentFiles.count())
                m_watcher.removePaths(currentFiles);
        }
    }
}

void MerSpecUpdater::onProjectAdded(Project *project)
{
    const QList<Kit *> merKits = MerSdkManager::instance()->merKits();
    foreach (Kit *kit, merKits) {
        if (project->supportsKit(kit)) {
            if (monitorProject(project))
                break;
        }
    }
}

void MerSpecUpdater::onProjectRemoved(Project *project)
{
    m_projectsToMonitor.removeOne(project);
    project->disconnect(this);
    QStringList currentFiles = m_projectFileMap.values(project);
    if (currentFiles.count())
        m_watcher.removePaths(currentFiles);
}

bool MerSpecUpdater::monitorProject(const Project *project)
{
    const Qt4Project *qt4Project = qobject_cast<const Qt4Project *>(project);
    if (!qt4Project)
        return false;
    m_projectsToMonitor.append(project);
    connect(qt4Project, SIGNAL(fileListChanged()), SLOT(monitorYamlFile()));
    monitorYamlFile(project);
    return true;
}

void MerSpecUpdater::onYamlFileChanged(const QString &yamlPath)
{
    m_progress.setProgressRange(0, 100);
    Core::FutureProgress *fp = Core::ICore::progressManager()->addTask(m_progress.future(),
                                            tr("Updating Spec"),
                                            QLatin1String("Mer.SpecUpdater"));
    fp->setKeepOnFinish(Core::FutureProgress::HideOnFinish);
    m_progress.reportStarted();

    const Project *project = m_projectFileMap.key(yamlPath);
    if (project) {
        Kit *kit = project->activeTarget()->kit();
        if (MerSdkManager::instance()->merKits().contains(kit)) {
            const Utils::FileName specifyFile = MerSpecifyKitInformation::specifyPath(kit);
            if (!specifyFile.isEmpty()) {
                const QFileInfo yamlFileInfo(yamlPath);
                const QString specFile = yamlFileInfo.path() + QLatin1String("/") + yamlFileInfo.baseName() + QLatin1String(".spec");
                QProcess specify;
                Utils::Environment env;
                MerSdkKitInformation sdkInfo;
                sdkInfo.addToEnvironment(kit, env);
                specify.setEnvironment(env.toStringList());
                specify.start(specifyFile.toString(), QStringList() << QLatin1String("-n") << QLatin1String("-o") << specFile << yamlPath);
                if (!specify.waitForFinished(3000))
                    Utils::SynchronousProcess::stopProcess(specify);
            }
        }
    }
    m_progress.setProgressValue(100);
    m_progress.reportFinished();
}

} // Internal
} // Mer
