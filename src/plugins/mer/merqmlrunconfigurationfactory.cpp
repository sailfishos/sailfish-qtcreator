/****************************************************************************
**
** Copyright (C) 2016 Jolla Ltd.
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

#include "merqmlrunconfigurationfactory.h"

#include "merconstants.h"
#include "merdevicefactory.h"
#include "merqmlrunconfiguration.h"

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
const char SAILFISHAPP_QML_CONFIG[] = "sailfishapp_qml";
} // anonymous namespace

MerQmlRunConfigurationFactory::MerQmlRunConfigurationFactory(QObject *parent)
    : IRunConfigurationFactory(parent)
{
}

QList<Core::Id> MerQmlRunConfigurationFactory::availableCreationIds(
        Target *parent, CreationMode mode) const
{
    Q_UNUSED(mode);

    QList<Core::Id>result;

    if (!canHandle(parent))
        return result;

    QmakeProject *qmakeProject = qobject_cast<QmakeProject *>(parent->project());
    if (!qmakeProject)
        return result;

    QmakeProFileNode *rootNode = qmakeProject->rootProjectNode();
    if (rootNode->projectType() == AuxTemplate &&
            rootNode->variableValue(ConfigVar).contains(QLatin1String(SAILFISHAPP_QML_CONFIG))) {
        result << MER_QMLRUNCONFIGURATION;
    }

    return result;
}

QString MerQmlRunConfigurationFactory::displayNameForId(Core::Id id) const
{
    if (id == MER_QMLRUNCONFIGURATION)
        return tr("QML Scene (on Sailfish OS Device)");

    return QString{};
}

bool MerQmlRunConfigurationFactory::canCreate(Target *parent, Core::Id id) const
{
    if (!canHandle(parent))
        return false;

    return id == MER_QMLRUNCONFIGURATION;
}

RunConfiguration *MerQmlRunConfigurationFactory::doCreate(Target *parent, Core::Id id)
{
    if (!canCreate(parent, id))
        return nullptr;

    if (id == MER_QMLRUNCONFIGURATION) {
        MerQmlRunConfiguration *config = new MerQmlRunConfiguration{parent, id};
        config->setDefaultDisplayName(displayNameForId(id));
        return config;
    }

    return nullptr;
}

bool MerQmlRunConfigurationFactory::canRestore(Target *parent, const QVariantMap &map) const
{
    if (!canHandle(parent))
        return false;

    return ProjectExplorer::idFromMap(map) == MER_QMLRUNCONFIGURATION;
}

RunConfiguration *MerQmlRunConfigurationFactory::doRestore(Target *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return nullptr;

    Core::Id id = ProjectExplorer::idFromMap(map);
    RunConfiguration *rc = new MerQmlRunConfiguration{parent, id};
    QTC_ASSERT(rc, return nullptr);
    if (rc->fromMap(map))
        return rc;

    delete rc;
    return nullptr;
}

bool MerQmlRunConfigurationFactory::canClone(Target *parent, RunConfiguration *source) const
{
    return canCreate(parent, source->id());
}

RunConfiguration *MerQmlRunConfigurationFactory::clone(Target *parent, RunConfiguration *source)
{
    if (!canClone(parent, source))
        return nullptr;

    MerQmlRunConfiguration *old = qobject_cast<MerQmlRunConfiguration *>(source);

    if (!old)
        return nullptr;

    return new MerQmlRunConfiguration{parent, old};
}

bool MerQmlRunConfigurationFactory::canHandle(Target *t) const
{
    return MerDeviceFactory::canCreate(DeviceTypeKitInformation::deviceTypeId(t->kit()));
}

} // Internal
} // Mer
