/****************************************************************************
**
** Copyright (C) 2012 - 2014 Jolla Ltd.
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

#ifndef MERDEPLOYCONFIGURATIONFACTORY_H
#define MERDEPLOYCONFIGURATIONFACTORY_H

#include <projectexplorer/deployconfiguration.h>

namespace Mer {
namespace Internal {

class MerDeployConfigurationFactory : public ProjectExplorer::DeployConfigurationFactory
{
    Q_OBJECT

public:
    explicit MerDeployConfigurationFactory(QObject *parent = 0);

    QList<Core::Id> availableCreationIds(ProjectExplorer::Target *parent) const override;
    QString displayNameForId(Core::Id id) const override;

    bool canCreate(ProjectExplorer::Target *parent, Core::Id id) const override;
    ProjectExplorer::DeployConfiguration *create(ProjectExplorer::Target *parent,
                                                 Core::Id id) override;

    bool canRestore(ProjectExplorer::Target *parent, const QVariantMap &map) const override;
    ProjectExplorer::DeployConfiguration *restore(ProjectExplorer::Target *parent,
                                                  const QVariantMap &map) override;

    bool canClone(ProjectExplorer::Target *parent,
                  ProjectExplorer::DeployConfiguration *source) const override;
    ProjectExplorer::DeployConfiguration *clone(ProjectExplorer::Target *parent,
                                                ProjectExplorer::DeployConfiguration *source) override;

private:
    bool canHandle(ProjectExplorer::Target *t) const;
};

} // namespace Internal
} // namespace Mer

#endif // MERDEPLOYCONFIGURATIONFACTORY_H
