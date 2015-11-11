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

#include "merdeployconfigurationfactory.h"

#include "merconstants.h"
#include "merdeployconfiguration.h"
#include "merdeploysteps.h"
#include "merdevicefactory.h"
#include "merrpmpackagingstep.h"
#include "meruploadandinstallrpmsteps.h"

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
    //TODO: canHandle(parent)
    Core::Id type = DeviceTypeKitInformation::deviceTypeId(parent->kit());
    if (type == Constants::MER_DEVICE_TYPE) {
        ids << MerMb2RpmBuildConfiguration::configurationId()
            << MerRsyncDeployConfiguration::configurationId()
            << MerRpmDeployConfiguration::configurationId();
            //<< MerRpmBuildDeployConfiguration::configurationId();
    }

    return ids;
}

QString MerDeployConfigurationFactory::displayNameForId(Core::Id id) const
{
    if (id == MerRsyncDeployConfiguration::configurationId())
        return MerRsyncDeployConfiguration::displayName();
    else if (id == MerRpmDeployConfiguration::configurationId())
        return MerRpmDeployConfiguration::displayName();
    else if (id == MerMb2RpmBuildConfiguration::configurationId())
        return MerMb2RpmBuildConfiguration::displayName();
    else if (id == MerRpmBuildDeployConfiguration::configurationId())
        return MerRpmBuildDeployConfiguration::displayName();

    return QString();
}

bool MerDeployConfigurationFactory::canCreate(Target *parent, Core::Id id) const
{
    if (canHandle(parent) && availableCreationIds(parent).contains(id))
        return true;

    return false;
}

DeployConfiguration *MerDeployConfigurationFactory::create(Target *parent, Core::Id id)
{
    QTC_ASSERT(canCreate(parent, id), return 0);

    DeployConfiguration *dc = 0;

     if (id == MerRpmDeployConfiguration::configurationId()) {
         dc = new MerRpmDeployConfiguration(parent, id);
         dc->stepList()->insertStep(0, new MerPrepareTargetStep(dc->stepList()));
         dc->stepList()->insertStep(1, new MerMb2RpmDeployStep(dc->stepList()));
     } else if (id == MerRsyncDeployConfiguration::configurationId()) {
         dc = new MerRsyncDeployConfiguration(parent, id);
         dc->stepList()->insertStep(0, new MerPrepareTargetStep(dc->stepList()));
         dc->stepList()->insertStep(1, new MerMb2RsyncDeployStep(dc->stepList()));
     } else if (id == MerMb2RpmBuildConfiguration::configurationId()) {
         dc = new MerMb2RpmBuildConfiguration(parent, id);
         dc->stepList()->insertStep(0, new MerMb2RpmBuildStep(dc->stepList()));
         dc->stepList()->insertStep(1, new MerRpmValidationStep(dc->stepList()));
         //dc->stepList()->insertStep(2, new MerUploadAndInstallRpmStep(dc->stepList()));
     } else if (id == MerRpmBuildDeployConfiguration::configurationId()) {
         dc = new MerRpmBuildDeployConfiguration(parent, id);
         dc->stepList()->insertStep(1, new MerRpmPackagingStep(dc->stepList()));
         dc->stepList()->insertStep(2, new MerUploadAndInstallRpmStep(dc->stepList()));
     }

    return dc;
}


bool MerDeployConfigurationFactory::canRestore(Target *parent, const QVariantMap &map) const
{
    return canCreate(parent, ProjectExplorer::idFromMap(map));
}

DeployConfiguration *MerDeployConfigurationFactory::restore(Target *parent, const QVariantMap &map)
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

    if (!dc->stepList()->contains(MerPrepareTargetStep::stepId())
            && id != MerMb2RpmBuildConfiguration::configurationId()) {
        dc->stepList()->insertStep(0, new MerPrepareTargetStep(dc->stepList()));
    }

    return dc;
}

bool MerDeployConfigurationFactory::canClone(Target *parent,
                                             DeployConfiguration *source) const
{
    return canCreate(parent, source->id());
}

DeployConfiguration *MerDeployConfigurationFactory::clone(
        Target *parent, DeployConfiguration *source)
{
    if (!canClone(parent, source))
        return 0;
    if (source->id() == MerRpmDeployConfiguration::configurationId()) {
        return new MerRpmDeployConfiguration(parent, qobject_cast<MerRpmDeployConfiguration *>(source));
    } else if(source->id() == MerRsyncDeployConfiguration::configurationId()){
        return new MerRsyncDeployConfiguration(parent, qobject_cast<MerRsyncDeployConfiguration *>(source));
    }else if(source->id() == MerMb2RpmBuildConfiguration::configurationId()){
        return new MerMb2RpmBuildConfiguration(parent, qobject_cast<MerMb2RpmBuildConfiguration *>(source));
    } else if(source->id() == MerRpmBuildDeployConfiguration::configurationId()){
        return new MerRpmBuildDeployConfiguration(parent, qobject_cast<MerRpmBuildDeployConfiguration *>(source));
    }

    return 0;
}

bool MerDeployConfigurationFactory::canHandle(Target *t) const
{
    return MerDeviceFactory::canCreate(DeviceTypeKitInformation::deviceTypeId(t->kit()));
}

} // Internal
} // Mer
