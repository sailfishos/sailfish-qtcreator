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

QList<BuildStepInfo> MerBuildStepFactory::availableSteps(BuildStepList *parent) const
{
    if (!qobject_cast<QmakeBuildConfiguration *>(parent->parent()))
        return {};

    return {{ MerSdkStartStep::stepId(), MerSdkStartStep::displayName() }};
}

BuildStep *MerBuildStepFactory::create(BuildStepList *parent, Core::Id id)
{
    Q_UNUSED(id);
    return new MerSdkStartStep(parent);
}

BuildStep *MerBuildStepFactory::clone(BuildStepList *parent, BuildStep *product)
{
    return new MerSdkStartStep(parent, qobject_cast<MerSdkStartStep *>(product));
}

} // namespace Internal
} // namespace Mer
