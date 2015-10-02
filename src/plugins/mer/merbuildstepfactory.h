/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
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

#ifndef MERBUILDSTEPFACTORY_H
#define MERBUILDSTEPFACTORY_H

#include <projectexplorer/buildstep.h>

namespace Mer {
namespace Internal {

class MerBuildStepFactory : public ProjectExplorer::IBuildStepFactory
{
    Q_OBJECT

public:
    explicit MerBuildStepFactory(QObject *parent = 0);

    QList<Core::Id> availableCreationIds(ProjectExplorer::BuildStepList *parent) const override;
    QString displayNameForId(Core::Id id) const override;

    bool canCreate(ProjectExplorer::BuildStepList *parent, Core::Id id) const override;
    ProjectExplorer::BuildStep *create(ProjectExplorer::BuildStepList *parent, Core::Id id) override;

    bool canRestore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map) const override;
    ProjectExplorer::BuildStep *restore(ProjectExplorer::BuildStepList *parent,
                                        const QVariantMap &map) override;

    bool canClone(ProjectExplorer::BuildStepList *parent,
                  ProjectExplorer::BuildStep *product) const override;
    ProjectExplorer::BuildStep *clone(ProjectExplorer::BuildStepList *parent,
                                      ProjectExplorer::BuildStep *product) override;
};

} // Internal
} // Mer

#endif // MERBUILDSTEPFACTORY_H

