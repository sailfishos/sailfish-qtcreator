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

#include "merdeploystepfactory.h"

#include "merdeployconfiguration.h"
#include "merdeploysteps.h"
#include "merrpminstaller.h"
#include "merrpmpackagingstep.h"
#include "meruploadandinstallrpmsteps.h"

#include <projectexplorer/buildsteplist.h>
#include <utils/qtcassert.h>

using namespace ProjectExplorer;

namespace Mer {
namespace Internal {

MerDeployStepFactory::MerDeployStepFactory(QObject *parent)
    : IBuildStepFactory(parent)
{
}

QList<BuildStepInfo> MerDeployStepFactory::availableSteps(BuildStepList *parent) const
{
    if (!qobject_cast<MerDeployConfiguration *>(parent->parent()))
        return {};

    // Intentionally omit MerEmulatorStartStep and MerConnectionTestStep - these
    // are available wrapped by MerPrepareTargetStep
    return {
        { MerPrepareTargetStep::stepId(), MerPrepareTargetStep::displayName() },
        { MerMb2RsyncDeployStep::stepId(), MerMb2RsyncDeployStep::displayName() },
        { MerMb2RpmDeployStep::stepId(), MerMb2RpmDeployStep::displayName() },
        { MerMb2RpmBuildStep::stepId(), MerMb2RpmBuildStep::displayName() },
        { MerRpmPackagingStep::stepId(), MerRpmPackagingStep::displayName() },
        { MerRpmValidationStep::stepId(), MerRpmValidationStep::displayName() },
        { MerUploadAndInstallRpmStep::stepId(), MerUploadAndInstallRpmStep::displayName() },
        { MerLocalRsyncDeployStep::stepId(), MerLocalRsyncDeployStep::displayName() },
    };
}

BuildStep *MerDeployStepFactory::create(BuildStepList *parent, Core::Id id)
{
    if (id == MerPrepareTargetStep::stepId())
        return new MerPrepareTargetStep(parent);
    if (id == MerMb2RsyncDeployStep::stepId())
        return new MerMb2RsyncDeployStep(parent);
    if (id == MerMb2RpmDeployStep::stepId())
        return new MerMb2RpmDeployStep(parent);
    if (id == MerMb2RpmBuildStep::stepId())
        return new MerMb2RpmBuildStep(parent);
    if (id == MerRpmPackagingStep::stepId())
        return new MerRpmPackagingStep(parent);
    if (id == MerRpmValidationStep::stepId())
        return new MerRpmValidationStep(parent);
    if (id == MerUploadAndInstallRpmStep::stepId())
        return new MerUploadAndInstallRpmStep(parent);
    if (id == MerLocalRsyncDeployStep::stepId())
        return new MerLocalRsyncDeployStep(parent);

    return 0;
}

BuildStep *MerDeployStepFactory::clone(BuildStepList *parent, BuildStep *product)
{
    if (MerEmulatorStartStep * const other = qobject_cast<MerEmulatorStartStep *>(product))
        return new MerEmulatorStartStep(parent, other);
    if (MerConnectionTestStep * const other = qobject_cast<MerConnectionTestStep *>(product))
        return new MerConnectionTestStep(parent, other);
    if (MerPrepareTargetStep * const other = qobject_cast<MerPrepareTargetStep *>(product))
        return new MerPrepareTargetStep(parent, other);
    if (MerMb2RsyncDeployStep * const other = qobject_cast<MerMb2RsyncDeployStep *>(product))
        return new MerMb2RsyncDeployStep(parent, other);
    if (MerMb2RpmDeployStep * const other = qobject_cast<MerMb2RpmDeployStep *>(product))
        return new MerMb2RpmDeployStep(parent, other);
    if (MerMb2RpmBuildStep * const other = qobject_cast<MerMb2RpmBuildStep *>(product))
        return new MerMb2RpmBuildStep(parent, other);
    if (MerRpmPackagingStep * const other = qobject_cast<MerRpmPackagingStep *>(product))
        return new MerRpmPackagingStep(parent, other);
    if (MerRpmValidationStep * const other = qobject_cast<MerRpmValidationStep *>(product))
        return new MerRpmValidationStep(parent, other);
    if (MerUploadAndInstallRpmStep * const other = qobject_cast<MerUploadAndInstallRpmStep *>(product))
        return new MerUploadAndInstallRpmStep(parent, other);
    if (MerLocalRsyncDeployStep * const other = qobject_cast<MerLocalRsyncDeployStep *>(product))
        return new MerLocalRsyncDeployStep(parent, other);

    return 0;
}

} // namespace Internal
} // namespace Mer
