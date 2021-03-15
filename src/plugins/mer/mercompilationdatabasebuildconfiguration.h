/****************************************************************************
**
** Copyright (C) 2020 Open Mobile Platform LLC.
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

#pragma once

#include <compilationdatabaseprojectmanager/compilationdatabaseproject.h>
#include <projectexplorer/makestep.h>

namespace Mer {
namespace Internal {

class MerCompilationDatabaseBuildConfiguration : public ProjectExplorer::BuildConfiguration
{
    Q_OBJECT

public:
    MerCompilationDatabaseBuildConfiguration(ProjectExplorer::Target *target, Utils::Id id);

    QList<ProjectExplorer::NamedWidget *> createSubConfigWidgets() override;
    void addToEnvironment(Utils::Environment &env) const override;
};

class MerCompilationDatabaseBuildConfigurationFactory
    : public ProjectExplorer::BuildConfigurationFactory
{
public:
    MerCompilationDatabaseBuildConfigurationFactory();
};

class MerCompilationDatabaseMakeStep : public ProjectExplorer::MakeStep
{
    Q_OBJECT

public:
    explicit MerCompilationDatabaseMakeStep(ProjectExplorer::BuildStepList *parent, Utils::Id id);
};

class MerCompilationDatabaseMakeStepFactory : public ProjectExplorer::BuildStepFactory
{
public:
    MerCompilationDatabaseMakeStepFactory();
};

} // namespace Internal
} // namespace Mer
