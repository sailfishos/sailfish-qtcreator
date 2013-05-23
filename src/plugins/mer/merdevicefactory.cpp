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

#include "merdevicefactory.h"
#include "merdeviceconfigurationwizard.h"
#include "merdevice.h"
#include "merconstants.h"

#include <utils/qtcassert.h>

namespace Mer {
namespace Internal {

MerDeviceFactory::MerDeviceFactory()
{
    setObjectName(QLatin1String("MerDeviceFactory"));
}

QString MerDeviceFactory::displayNameForId(Core::Id type) const
{
    if (type == Constants::MER_DEVICE_TYPE_I486)
        return tr("Mer i486 Device");
    if (type == Constants::MER_DEVICE_TYPE_ARM)
        return tr("Mer ARM Device");
    return QString();
}

bool MerDeviceFactory::canCreate(Core::Id type)
{
    return type == Core::Id(Constants::MER_DEVICE_TYPE_I486) ||
            type == Core::Id(Constants::MER_DEVICE_TYPE_ARM);
}


QList<Core::Id> MerDeviceFactory::availableCreationIds() const
{
    return QList<Core::Id>() << Core::Id(Constants::MER_DEVICE_TYPE_I486)
                             << Core::Id(Constants::MER_DEVICE_TYPE_ARM);
}

ProjectExplorer::IDevice::Ptr MerDeviceFactory::create(Core::Id id) const
{
    QTC_ASSERT(canCreate(id), return ProjectExplorer::IDevice::Ptr());
    MerDeviceConfigurationWizard wizard(id);
    if (wizard.exec() != QDialog::Accepted)
        return ProjectExplorer::IDevice::Ptr();
    return wizard.device();
}

bool MerDeviceFactory::canRestore(const QVariantMap &map) const
{
    return canCreate(ProjectExplorer::IDevice::typeFromMap(map));
}

ProjectExplorer::IDevice::Ptr MerDeviceFactory::restore(const QVariantMap &map) const
{
    QTC_ASSERT(canRestore(map), return ProjectExplorer::IDevice::Ptr());
    const ProjectExplorer::IDevice::Ptr device = MerDevice::create();
    device->fromMap(map);
    return device;
}

} // Internal
} // Mer
