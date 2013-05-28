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

#ifndef MERPROJECTLISTENER_H
#define MERPROJECTLISTENER_H

#include <QObject>

namespace ProjectExplorer {
class Project;
class Target;
}

namespace Qt4ProjectManager {
class Qt4Project;
}

namespace Mer {
namespace Internal {

class MerProjectListener : public QObject
{
    Q_OBJECT
public:
    explicit MerProjectListener(QObject *parent = 0);
    virtual ~MerProjectListener() {}

protected slots:
    virtual void init();
    virtual bool handleProject(const Qt4ProjectManager::Qt4Project *project) = 0;

private slots:
    virtual void onTargetAddedToProject(ProjectExplorer::Target *target);
    virtual void onTargetRemovedFromProject(ProjectExplorer::Target *target);
    virtual void onProjectAdded(ProjectExplorer::Project *project);
    virtual void onProjectRemoved(ProjectExplorer::Project *project);

private:
    bool handleProject_private(const ProjectExplorer::Project *project);

protected:
    QList<const ProjectExplorer::Project *> m_projectsToHandle;
};

} // Internal
} // Mer

#endif // MERPROJECTLISTENER_H
