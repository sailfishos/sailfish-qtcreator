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

QList<Core::Id> MerDeployStepFactory::availableCreationIds(BuildStepList *parent) const
{
    QList<Core::Id> ids;
    if (!qobject_cast<MerDeployConfiguration *>(parent->parent()))
        return ids;

    // Intentionally omit MerEmulatorStartStep and MerConnectionTestStep - these
    // are available wrapped by MerPrepareTargetStep
    ids << MerPrepareTargetStep::stepId();
    ids << MerMb2RsyncDeployStep::stepId();
    ids << MerMb2RpmDeployStep::stepId();
    ids << MerMb2RpmBuildStep::stepId();
    ids << MerRpmPackagingStep::stepId();
    ids << MerRpmValidationStep::stepId();
    ids << MerUploadAndInstallRpmStep::stepId();
    ids << MerLocalRsyncDeployStep::stepId();

    return ids;
}

QString MerDeployStepFactory::displayNameForId(Core::Id id) const
{
    if (id == MerPrepareTargetStep::stepId())
        return MerPrepareTargetStep::displayName();
    if (id == MerMb2RsyncDeployStep::stepId())
        return MerMb2RsyncDeployStep::displayName();
    if (id == MerMb2RpmDeployStep::stepId())
        return MerMb2RpmDeployStep::displayName();
    if (id == MerMb2RpmBuildStep::stepId())
        return MerMb2RpmBuildStep::displayName();
    if (id == MerRpmPackagingStep::stepId())
        return MerRpmPackagingStep::displayName();
    if (id == MerRpmValidationStep::stepId())
        return MerRpmValidationStep::displayName();
    if (id == MerUploadAndInstallRpmStep::stepId())
        return MerUploadAndInstallRpmStep::displayName();
    if (id == MerLocalRsyncDeployStep::stepId())
        return MerLocalRsyncDeployStep::displayName();

    return QString();
}

bool MerDeployStepFactory::canCreate(BuildStepList *parent, Core::Id id) const
{
    return availableCreationIds(parent).contains(id) && !parent->contains(id);
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

bool MerDeployStepFactory::canRestore(BuildStepList *parent, const QVariantMap &map) const
{
    return canCreate(parent, idFromMap(map));
}

BuildStep *MerDeployStepFactory::restore(BuildStepList *parent, const QVariantMap &map)
{
    QTC_ASSERT(canRestore(parent, map), return 0);
    BuildStep * const step = create(parent, idFromMap(map));
    if (!step->fromMap(map)) {
        delete step;
        return 0;
    }
    return step;
}

bool MerDeployStepFactory::canClone(BuildStepList *parent, BuildStep *product) const
{
    return canCreate(parent, product->id());
}

BuildStep *MerDeployStepFactory::clone(BuildStepList *parent, BuildStep *product)
{
    QTC_ASSERT(canClone(parent, product), return 0);
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
