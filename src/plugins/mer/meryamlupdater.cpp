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

#include "meryamlupdater.h"
#include "mersdkmanager.h"

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>

#include <qt4projectmanager/qt4project.h>
#include <qt4projectmanager/qt4nodes.h>

#include <qtsupport/qtkitinformation.h>

#include <utils/fileutils.h>

using namespace ProjectExplorer;
using namespace QtSupport;
using namespace Qt4ProjectManager;

const char SECTION_START[] = "#This section is overwritten by Qt Creator. Do not remove the comments.";
const char SECTION_END[] = "#Sections ends.";

namespace Mer {
namespace Internal {

MerYamlUpdater::MerYamlUpdater(QObject *parent) :
    QObject(parent)
{
    connect(MerSdkManager::instance(), SIGNAL(initialized()), SLOT(init()));
}

void MerYamlUpdater::init()
{
    const SessionManager *sessionManager = ProjectExplorerPlugin::instance()->session();

    const QList<Kit *> merKits = MerSdkManager::instance()->merKits();
    const QList<Project *> projects = sessionManager->projects();
    // O(n2)
    foreach (const Project *project, projects) {
        connect(project, SIGNAL(addedTarget(ProjectExplorer::Target*)),
                SLOT(onTargetAddedToProject(ProjectExplorer::Target*)));
        connect(project, SIGNAL(removedTarget(ProjectExplorer::Target*)),
                SLOT(onTargetRemovedFromProject(ProjectExplorer::Target*)));
        foreach (Kit *kit, merKits) {
            if (project->supportsKit(kit)) {
                const Qt4Project *qt4Project = qobject_cast<const Qt4Project *>(project);
                if (!qt4Project)
                    continue;
                m_projectsToMonitor << project;
                connect(qt4Project, SIGNAL(proFilesEvaluated()), SLOT(onProFilesEvaluated()));
                onProFilesEvaluated(project);
                break;
            }
        }
    }

    connect(sessionManager, SIGNAL(projectAdded(ProjectExplorer::Project*)),
            SLOT(onProjectAdded(ProjectExplorer::Project*)));
    connect(sessionManager, SIGNAL(projectRemoved(ProjectExplorer::Project*)),
            SLOT(onProjectRemoved(ProjectExplorer::Project*)));
}

void MerYamlUpdater::onProFilesEvaluated(const Project *proj)
{

    const Qt4Project *project = proj ? qobject_cast<const Qt4Project *>(proj)
                                     : qobject_cast<const Qt4Project *>(sender());
    if (!project || !m_projectsToMonitor.contains(project))
        return;

    QStringList targetPaths;
    const QList<Qt4ProFileNode *> proFiles = project->allProFiles();
    foreach (const Qt4ProFileNode *node, proFiles) {
        const InstallsList list = node->installsList();
        targetPaths.append(list.targetPath);
        foreach (InstallsItem item, list.items)
            targetPaths.append(item.path);
    }
    targetPaths.removeDuplicates();
    // Get the yaml file
    const QStringList allFiles = project->files(Project::ExcludeGeneratedFiles);
    QString yamlFile;
    foreach (const QString &file, allFiles) {
        if (file.endsWith(QLatin1String(".yaml"))) {
            yamlFile = file;
            break;
        }
    }
    QByteArray newContent;
    {
        Utils::FileReader yamlReader;
        if (!yamlReader.fetch(yamlFile, QIODevice::ReadOnly))
            return;
        const QByteArray data = yamlReader.data();
        // "Files:(.|(\\s+-\\s+)|\\n)*(?:%1)(.|(\\s+-\\s+)|\\n)*(?:%2)"
        QRegExp rxp(QString::fromLatin1("(Files:.*)(?:%1\n)(.*)(?:%2)(.*:?)").arg(QLatin1String(SECTION_START), QLatin1String(SECTION_END)));
        rxp.setMinimal(true);
        int pos = 0;
        if ((pos = rxp.indexIn(QString::fromUtf8(data), pos)) != -1) {
            newContent = data.left(pos);
            newContent.append(rxp.cap(1).toUtf8());

            newContent.append(SECTION_START);
            newContent.append("\n");
            QString prefix;
            const QRegExp rxPrefix(QLatin1String("\n+(\\s*-\\s*)"));
            if (rxPrefix.indexIn(rxp.cap(1)) != -1)
                prefix = rxPrefix.cap(1);
            else if (rxPrefix.indexIn(rxp.cap(3)) != -1)
                prefix = rxPrefix.cap(1);
            else
                prefix = QLatin1String("  - ");
            foreach (const QString &target, targetPaths)
                newContent.append(prefix.toUtf8() + '"' + target.toUtf8() + "\"\n");
            newContent.append(SECTION_END);
            pos += rxp.matchedLength();
            newContent.append(data.mid(pos));
        }
    }
    {
        Utils::FileSaver yamlSaver(yamlFile, QIODevice::WriteOnly);
        yamlSaver.write(newContent);
        yamlSaver.finalize();
    }
}

void MerYamlUpdater::onTargetAddedToProject(Target *target)
{
    const Project *project = target->project();
    if (MerSdkManager::instance()->merKits().contains(target->kit())
            && !m_projectsToMonitor.contains(project)) {
        const Qt4Project *qt4Project = qobject_cast<const Qt4Project *>(project);
        if (qt4Project) {
            m_projectsToMonitor.append(project);
            connect(qt4Project, SIGNAL(proFilesEvaluated()), SLOT(onProFilesEvaluated()));
            onProFilesEvaluated(project);
        }
    }
}

void MerYamlUpdater::onTargetRemovedFromProject(Target *target)
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
        }
    }
}

void MerYamlUpdater::onProjectAdded(Project *project)
{
    const QList<Kit *> merKits = MerSdkManager::instance()->merKits();
    foreach (Kit *kit, merKits) {
        if (project->supportsKit(kit)) {
            const Qt4Project *qt4Project = qobject_cast<const Qt4Project *>(project);
            if (!qt4Project)
                continue;
            m_projectsToMonitor << project;
            connect(qt4Project, SIGNAL(proFilesEvaluated()), SLOT(onProFilesEvaluated()));
            onProFilesEvaluated(project);
            break;
        }
    }
}

void MerYamlUpdater::onProjectRemoved(Project *project)
{
    m_projectsToMonitor.removeOne(project);
    project->disconnect(this);
}

} // Internal
} // Mer
