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

#ifndef MERYAMLUPDATER_H
#define MERYAMLUPDATER_H

#include <QObject>

namespace ProjectExplorer {
class Project;
class Target;
}

namespace Mer {
namespace Internal {

class MerYamlUpdater : public QObject
{
    Q_OBJECT
public:
    explicit MerYamlUpdater(QObject *parent = 0);

private slots:
    void init();
    void onProFilesEvaluated(const ProjectExplorer::Project *project = 0);
    void onTargetAddedToProject(ProjectExplorer::Target *target);
    void onTargetRemovedFromProject(ProjectExplorer::Target *target);
    void onProjectAdded(ProjectExplorer::Project *project);
    void onProjectRemoved(ProjectExplorer::Project *project);

private:
    bool monitorProject(const ProjectExplorer::Project *project);

private:
    QList<const ProjectExplorer::Project *> m_projectsToMonitor;
};

} // Internal
} // Mer

#endif // MERYAMLUPDATER_H
