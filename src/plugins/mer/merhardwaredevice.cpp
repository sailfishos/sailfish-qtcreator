/****************************************************************************
**
** Copyright (C) 2012-2015,2019 Jolla Ltd.
** Copyright (C) 2019 Open Mobile Platform LLC.
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

#include "merhardwaredevice.h"

#include "merconstants.h"
#include "merdevicefactory.h"
#include "merhardwaredevicewidget.h"
#include "merhardwaredevicewizardpages.h"
#include "mersdkmanager.h"

#include <sfdk/sfdkconstants.h>

#include <projectexplorer/devicesupport/devicemanager.h>
#include <remotelinux/remotelinux_constants.h>
#include <ssh/sshconnection.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QProgressDialog>
#include <QTimer>
#include <QWizard>

using namespace ProjectExplorer;
using namespace RemoteLinux;
using namespace QSsh;
using namespace Utils;

namespace Mer {
namespace Internal {

MerHardwareDevice::MerHardwareDevice()
{
    setMachineType(IDevice::Hardware);

    PortList defaultQmlLivePorts;
    defaultQmlLivePorts.addPort(Utils::Port(Sfdk::Constants::DEFAULT_QML_LIVE_PORT));
    setQmlLivePorts(defaultQmlLivePorts);
}

IDevice::Ptr MerHardwareDevice::clone() const
{
    return Ptr(new MerHardwareDevice(*this));
}

IDeviceWidget *MerHardwareDevice::createWidget()
{
    return new MerHardwareDeviceWidget(sharedFromThis());
}

} // Internal
} // Mer
