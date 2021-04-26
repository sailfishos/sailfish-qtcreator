/****************************************************************************
**
** Copyright (C) 2012-2015,2017 Jolla Ltd.
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

#include "merhardwaredevicewizard.h"

#include <sfdk/sdk.h>

#include <ssh/sshconnection.h>

using namespace ProjectExplorer;
using namespace Sfdk;
using namespace QSsh;

namespace Mer {
namespace Internal {

MerHardwareDeviceWizard::MerHardwareDeviceWizard(QWidget *parent)
    : Utils::Wizard(parent),
      m_selectionPage(this),
      m_keyDeploymentPage(this),
      m_packageSingKeyDeploymentPage(this),
      m_connectionTestPage(this),
      m_setupPage(this),
      m_finalPage(this)
{
    setWindowTitle(tr("New %1 Hardware Device Setup").arg(Sdk::osVariant()));
    addPage(&m_selectionPage);
    addPage(&m_keyDeploymentPage);
    addPage(&m_packageSingKeyDeploymentPage);
    addPage(&m_connectionTestPage);
    addPage(&m_setupPage);
    addPage(&m_finalPage);
    m_finalPage.setCommitPage(true);

    // Avoid shaking
    m_selectionPage.setMinimumSize(m_keyDeploymentPage.sizeHint());

    m_device = MerHardwareDevice::create();
    m_device->setupId(IDevice::ManuallyAdded, Utils::Id());

    m_selectionPage.setDevice(m_device);
    m_keyDeploymentPage.setDevice(m_device);
    m_packageSingKeyDeploymentPage.setDevice(m_device);
    m_connectionTestPage.setDevice(m_device);
    m_setupPage.setDevice(m_device);
}

MerHardwareDeviceWizard::~MerHardwareDeviceWizard()
{

}

MerHardwareDevice::Ptr MerHardwareDeviceWizard::device() const
{
    return m_device;
}

} // Internal
} // Mer
