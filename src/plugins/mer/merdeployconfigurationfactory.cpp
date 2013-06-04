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

#include "merdeployconfigurationfactory.h"
#include "merdeployconfiguration.h"
#include "merconstants.h"
#include "meremulatorstartstep.h"
#include "merdevicefactory.h"
#include "merdeploysteps.h"

#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/buildstep.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/target.h>
#include <remotelinux/genericdirectuploadstep.h>

#include <utils/qtcassert.h>

using namespace ProjectExplorer;

namespace Mer {
namespace Internal {

MerDeployConfigurationFactory::MerDeployConfigurationFactory(QObject *parent)
    : DeployConfigurationFactory(parent)
{
}

QList<Core::Id> MerDeployConfigurationFactory::availableCreationIds(Target *parent) const
{
    QList<Core::Id> ids;

    if (canHandle(parent))
           ids << MerRsyncDeployConfiguration::configurationId() << MerRpmDeployConfiguration::configurationId();


    return ids;
}

QString MerDeployConfigurationFactory::displayNameForId(const Core::Id id) const
{
    if (id == MerRsyncDeployConfiguration::configurationId())
        return MerRsyncDeployConfiguration::displayName();
    else if (id == MerRpmDeployConfiguration::configurationId())
        return MerRpmDeployConfiguration::displayName();
    return QString();
}

bool MerDeployConfigurationFactory::canCreate(Target *parent, const Core::Id id) const
{
    if (canHandle(parent) && availableCreationIds(parent).contains(id))
        return true;

    return false;
}

DeployConfiguration *MerDeployConfigurationFactory::create(Target *parent, const Core::Id id)
{
    QTC_ASSERT(canCreate(parent, id), return 0);

    ProjectExplorer::DeployConfiguration *dc = 0;

     if (id == MerRpmDeployConfiguration::configurationId()) {
         dc = new MerRpmDeployConfiguration(parent, id);
         dc->stepList()->insertStep(0, new MerEmulatorStartStep(dc->stepList()));
         dc->stepList()->insertStep(1, new MerRpmDeployStep(dc->stepList()));
     } else if (id == MerRsyncDeployConfiguration::configurationId()) {
         dc = new MerRsyncDeployConfiguration(parent, id);
         dc->stepList()->insertStep(0, new MerEmulatorStartStep(dc->stepList()));
         dc->stepList()->insertStep(1, new MerRsyncDeployStep(dc->stepList()));
     }

    return dc;
}


bool MerDeployConfigurationFactory::canRestore(ProjectExplorer::Target *parent,
                                               const QVariantMap &map) const
{
    return canCreate(parent, ProjectExplorer::idFromMap(map));
}

ProjectExplorer::DeployConfiguration *MerDeployConfigurationFactory::restore(
        ProjectExplorer::Target *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;

    Core::Id id = ProjectExplorer::idFromMap(map);

    MerDeployConfiguration * const dc = qobject_cast<MerDeployConfiguration *>(create(parent, id));
    QTC_ASSERT(dc, return 0);
    if (!dc->fromMap(map)) {
        delete dc;
        return 0;
    }
    return dc;
}

bool MerDeployConfigurationFactory::canClone(ProjectExplorer::Target *parent,
                                             ProjectExplorer::DeployConfiguration *source) const
{
    return canCreate(parent, source->id());
}

ProjectExplorer::DeployConfiguration *MerDeployConfigurationFactory::clone(
        ProjectExplorer::Target *parent, ProjectExplorer::DeployConfiguration *source)
{

    if (!canClone(parent, source))
        return 0;
    if (source->id() == MerRpmDeployConfiguration::configurationId()) {
        return new MerRpmDeployConfiguration(parent, qobject_cast<MerRpmDeployConfiguration *>(source));
    }else if(source->id() == MerRsyncDeployConfiguration::configurationId()){
        return new MerRsyncDeployConfiguration(parent, qobject_cast<MerRsyncDeployConfiguration *>(source));
    }
    return 0;
}

bool MerDeployConfigurationFactory::canHandle(ProjectExplorer::Target *t) const
{
    return MerDeviceFactory::canCreate(ProjectExplorer::DeviceTypeKitInformation::deviceTypeId(t->kit()));
}

} // Internal
} // Mer
