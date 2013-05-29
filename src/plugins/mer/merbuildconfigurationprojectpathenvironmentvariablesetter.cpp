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

#include "merbuildconfigurationprojectpathenvironmentvariablesetter.h"
#include "mersdkmanager.h"
#include "merconstants.h"

#include <projectexplorer/target.h>
#include <projectexplorer/buildconfiguration.h>

#include <qt4projectmanager/qt4project.h>

#include <coreplugin/icore.h>

using namespace ProjectExplorer;
using namespace Qt4ProjectManager;

namespace Mer {
namespace Internal {

void MerBuildConfigurationProjectPathEnvironmentVariableSetter::onTargetAddedToProject(Target *target)
{
    MerProjectListener::onTargetAddedToProject(target);
    const QList<Kit*> merKits = MerSdkManager::instance()->merKits();
    if (!merKits.contains(target->kit()))
        return;
    Qt4Project *qt4Project = qobject_cast<Qt4Project*>(target->project());
    if (!qt4Project)
        return;
    addProjectPathEnvironmentVariableToMerBuildConfigurationsOfTarget(target);
}

void MerBuildConfigurationProjectPathEnvironmentVariableSetter::onProjectAdded(Project *project)
{
    Qt4Project *qt4Project = qobject_cast<Qt4Project*>(project);
    if (!qt4Project)
        return;
    const QList<Kit*> merKits = MerSdkManager::instance()->merKits();
    foreach (Target *target, qt4Project->targets()) {
        if (!merKits.contains(target->kit()))
            continue;
        addProjectPathEnvironmentVariableToMerBuildConfigurationsOfTarget(target);
    }
}

void MerBuildConfigurationProjectPathEnvironmentVariableSetter::addProjectPathEnvironmentVariableToMerBuildConfigurationsOfTarget(Target *target)
{
    QList<Utils::EnvironmentItem> envVars;
    envVars.append(Utils::EnvironmentItem(QLatin1String(Constants::MER_PROJECTPATH_ENVVAR_NAME),
                                          target->project()->projectDirectory()));
    foreach (BuildConfiguration *buildConfiguration, target->buildConfigurations())
        buildConfiguration->setUserEnvironmentChanges(envVars);
}

} // Internal
} // Mer
