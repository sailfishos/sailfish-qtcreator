/****************************************************************************
**
** Copyright (C) 2012-2015,2017-2019 Jolla Ltd.
** Copyright (C) 2019-2020 Open Mobile Platform LLC.
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

#include "merconstants.h"
#include "meremulatordevice.h"
#include "merhardwaredevice.h"
#include "merhardwaredevicewizard.h"
#include "mericons.h"

#include <sfdk/sdk.h>

#include <coreplugin/icore.h>
#include <utils/qtcassert.h>

using namespace Core;
using namespace ProjectExplorer;
using namespace Sfdk;
using namespace Utils;

namespace Mer {
namespace Internal {

MerDeviceFactory *MerDeviceFactory::s_instance= 0;

MerDeviceFactory *MerDeviceFactory::instance()
{
    return s_instance;
}

MerDeviceFactory::MerDeviceFactory()
    : IDeviceFactory(Constants::MER_DEVICE_TYPE)
{
    QTC_CHECK(!s_instance);
    s_instance = this;
    setDisplayName(MerDevice::tr("%1 Device").arg(Sdk::osVariant()));
    setIcon(Utils::creatorTheme()->flag(Utils::Theme::FlatSideBarIcons)
            ? Utils::Icon::combinedIcon({Icons::MER_DEVICE_FLAT,
                                         Icons::MER_DEVICE_FLAT_SMALL})
            : Icons::MER_DEVICE_CLASSIC.icon());
    setCanCreate(true);
}

MerDeviceFactory::~MerDeviceFactory()
{
    s_instance = 0;
}

IDevice::Ptr MerDeviceFactory::create() const
{
    MerHardwareDeviceWizard wizard(ICore::dialogParent());
    if (wizard.exec() != QDialog::Accepted)
        return IDevice::Ptr();
    return wizard.device();
}

bool MerDeviceFactory::canRestore(const QVariantMap &map) const
{
    // Hack
    if (MerDevice::workaround_machineTypeFromMap(map) == IDevice::Emulator)
        const_cast<MerDeviceFactory *>(this)->setConstructionFunction(MerEmulatorDevice::create);
    else
        const_cast<MerDeviceFactory *>(this)->setConstructionFunction(MerHardwareDevice::create);

    // Discard emulator device configurations from pre-libsfdk times - there is no matching Sfdk
    // device initially and so it would fail soon in MerEmulatorDevice::fromMap(). This is not
    // necessary with HW devices - for those the synchronization between QtC and Sfdk happens
    // bidirectionally.
    if (MerDevice::workaround_machineTypeFromMap(map) == IDevice::Emulator) {
        const Utils::Id id = IDevice::idFromMap(map);
        const QString sdkId = MerEmulatorDevice::toSdkId(id);
        if (!Sdk::device(sdkId))
            return false;
    }

    return true;
}

} // Internal
} // Mer
