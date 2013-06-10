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
#include "yamldocument.h"

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

using namespace ProjectExplorer;
using namespace QtSupport;
using namespace Qt4ProjectManager;

const char SECTION_START[] = "# This section is overwritten by Qt Creator. Do not remove the comments.";
const char SECTION_END[] = "# Sections ends.";

namespace Mer {
namespace Internal {

void MerYamlUpdater::onProFilesEvaluated(const Project *proj)
{
    const Qt4Project *project = proj ? qobject_cast<const Qt4Project *>(proj)
                                     : qobject_cast<const Qt4Project *>(sender());
    if (!project || !m_projectsToHandle.contains(project))
        return;

    m_progress.setProgressRange(0, 100);
    Core::FutureProgress *fp = Core::ICore::progressManager()->addTask(m_progress.future(),
                                            tr("Updating YAML"),
                                            QLatin1String("Mer.YAMLUpdater"));
    fp->setKeepOnFinish(Core::FutureProgress::HideOnFinish);
    m_progress.reportStarted();
    QStringList targetPaths;
    const QList<Qt4ProFileNode *> proFiles = project->allProFiles();
    foreach (const Qt4ProFileNode *node, proFiles) {
        const InstallsList list = node->installsList();
        targetPaths.append(list.targetPath);
        foreach (InstallsItem item, list.items)
            targetPaths.append(item.path);
    }
    targetPaths.removeDuplicates();
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
    if (!yamlFile.isEmpty())
        YamlDocument::updateFiles(targetPaths, yamlFile);
    m_progress.setProgressValue(100);
    m_progress.reportFinished();
}

bool MerYamlUpdater::handleProject(Qt4Project *qt4Project)
{
    if (m_projectsToHandle.contains(qt4Project))
        return false;
    m_projectsToHandle.append(qt4Project);
    connect(qt4Project, SIGNAL(proFilesEvaluated()), SLOT(onProFilesEvaluated()));
    onProFilesEvaluated(qt4Project);
    return true;
}

bool MerYamlUpdater::forgetProject(Project *project)
{
    m_projectsToHandle.removeOne(project);
    project->disconnect(this);
    return true;
}

} // Internal
} // Mer
