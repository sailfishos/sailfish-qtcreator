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

#include "merrunconfigurationfactory.h"

#include "merconstants.h"
#include "merdevicefactory.h"
#include "merrunconfiguration.h"

#include <projectexplorer/buildtargetinfo.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/target.h>
#include <qmakeprojectmanager/qmakeproject.h>
#include <utils/qtcassert.h>

using namespace Mer::Constants;
using namespace ProjectExplorer;
using namespace QmakeProjectManager;
using namespace Utils;

namespace Mer {
namespace Internal {

namespace {
QString stringFromId(Core::Id id)
{
    return id.suffixAfter(MER_RUNCONFIGURATION_PREFIX);
}
} // Anonymous

MerRunConfigurationFactory::MerRunConfigurationFactory(QObject *parent)
    : IRunConfigurationFactory(parent)
{
}

QList<Core::Id> MerRunConfigurationFactory::availableCreationIds(
        Target *parent, CreationMode mode) const
{
    Q_UNUSED(mode);

    QList<Core::Id>result;
    if (!canHandle(parent))
        return result;

    QmakeProject *qmakeProject =
            qobject_cast<QmakeProject *>(parent->project());
    if (!qmakeProject)
        return result;

    const Core::Id base = Core::Id(MER_RUNCONFIGURATION_PREFIX);
    foreach (const BuildTargetInfo &bti, parent->applicationTargets().list)
        result << base.withSuffix(bti.targetName);

    return result;
}

QString MerRunConfigurationFactory::displayNameForId(Core::Id id) const
{
    if (id.toString().startsWith(QLatin1String(MER_RUNCONFIGURATION_PREFIX)))
        return tr("%1 (on Mer Device)").arg(stringFromId(id));

    return QString();
}

bool MerRunConfigurationFactory::canCreate(Target *parent, Core::Id id) const
{
    if (!canHandle(parent))
        return false;

    return parent->applicationTargets().hasTarget(stringFromId(id));
}

RunConfiguration *MerRunConfigurationFactory::doCreate(
        Target *parent, Core::Id id)
{
    if (!canCreate(parent, id))
        return 0;

    if (id.toString().startsWith(QLatin1String(MER_RUNCONFIGURATION_PREFIX))) {
        MerRunConfiguration *config = new MerRunConfiguration(parent, id, stringFromId(id));
        config->setDefaultDisplayName(displayNameForId(id));
        return config;
    }

    return 0;
}

bool MerRunConfigurationFactory::canRestore(Target *parent,
                                            const QVariantMap &map) const
{
    if (!canHandle(parent))
        return false;

    return ProjectExplorer::idFromMap(map).toString().startsWith(
                QLatin1String(MER_RUNCONFIGURATION_PREFIX));
}

RunConfiguration *MerRunConfigurationFactory::doRestore(
        Target *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;

    Core::Id id = ProjectExplorer::idFromMap(map);
    RunConfiguration *rc = new MerRunConfiguration(parent, id, stringFromId(id));
    QTC_ASSERT(rc, return 0);
    if (rc->fromMap(map))
        return rc;

    delete rc;
    return 0;
}

bool MerRunConfigurationFactory::canClone(Target *parent,
                                          RunConfiguration *source) const
{
    return canCreate(parent, source->id());
}

RunConfiguration *MerRunConfigurationFactory::clone(
        Target *parent, RunConfiguration *source)
{
    if (!canClone(parent, source))
        return 0;

    MerRunConfiguration *old = qobject_cast<MerRunConfiguration *>(source);

    if (!old)
        return 0;

    return new MerRunConfiguration(parent, old);
}

bool MerRunConfigurationFactory::canHandle(Target *t) const
{
    return MerDeviceFactory::canCreate(DeviceTypeKitInformation::deviceTypeId(t->kit()));
}

} // Internal
} // Mer
