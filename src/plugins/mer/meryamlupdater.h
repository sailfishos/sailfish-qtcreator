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

#include "merprojectlistener.h"

#include <QObject>
#include <QFutureInterface>

namespace Mer {
namespace Internal {

class MerYamlUpdater : public MerProjectListener
{
    Q_OBJECT

private slots:
    void onProFilesEvaluated(const ProjectExplorer::Project *project = 0);

private:
    bool handleProject(Qt4ProjectManager::Qt4Project *qt4Project);
    bool forgetProject(ProjectExplorer::Project *project);

private:
    QFutureInterface<void> m_progress;
    QList<const ProjectExplorer::Project*> m_projectsToHandle;
};

} // Internal
} // Mer

#endif // MERYAMLUPDATER_H
