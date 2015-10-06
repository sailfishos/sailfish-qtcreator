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

#include "merbuildstepfactory.h"

#include "merbuildsteps.h"

#include <projectexplorer/buildsteplist.h>
#include <qmakeprojectmanager/qmakebuildconfiguration.h>
#include <utils/qtcassert.h>

using namespace ProjectExplorer;
using namespace QmakeProjectManager;

namespace Mer {
namespace Internal {

MerBuildStepFactory::MerBuildStepFactory(QObject *parent)
    : IBuildStepFactory(parent)
{
}

QList<Core::Id> MerBuildStepFactory::availableCreationIds(BuildStepList *parent) const
{
    QList<Core::Id> ids;
    if (!qobject_cast<QmakeBuildConfiguration *>(parent->parent()))
        return ids;

    ids << MerSdkStartStep::stepId();

    return ids;
}

QString MerBuildStepFactory::displayNameForId(Core::Id id) const
{
    if (id == MerSdkStartStep::stepId())
        return MerSdkStartStep::displayName();

    return QString();
}

bool MerBuildStepFactory::canCreate(BuildStepList *parent, Core::Id id) const
{
    return availableCreationIds(parent).contains(id) && !parent->contains(id);
}

BuildStep *MerBuildStepFactory::create(BuildStepList *parent, Core::Id id)
{
    if (id == MerSdkStartStep::stepId())
        return new MerSdkStartStep(parent);

    return 0;
}

bool MerBuildStepFactory::canRestore(BuildStepList *parent, const QVariantMap &map) const
{
    return canCreate(parent, idFromMap(map));
}

BuildStep *MerBuildStepFactory::restore(BuildStepList *parent, const QVariantMap &map)
{
    QTC_ASSERT(canRestore(parent, map), return 0);
    BuildStep * const step = create(parent, idFromMap(map));
    if (!step->fromMap(map)) {
        delete step;
        return 0;
    }
    return step;
}

bool MerBuildStepFactory::canClone(BuildStepList *parent, BuildStep *product) const
{
    return canCreate(parent, product->id());
}

BuildStep *MerBuildStepFactory::clone(BuildStepList *parent, BuildStep *product)
{
    QTC_ASSERT(canClone(parent, product), return 0);
    if (MerSdkStartStep * const other = qobject_cast<MerSdkStartStep *>(product))
        return new MerSdkStartStep(parent, other);

    return 0;
}

} // namespace Internal
} // namespace Mer
