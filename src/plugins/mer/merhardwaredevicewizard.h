/****************************************************************************
**
** Copyright (C) 2012-2015 Jolla Ltd.
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

#ifndef MERHARDWAREDEVICEWIZARD_H
#define MERHARDWAREDEVICEWIZARD_H

#include "merhardwaredevicewizardpages.h"

#include <projectexplorer/abi.h>
#include <remotelinux/genericlinuxdeviceconfigurationwizardpages.h>
#include <utils/wizard.h>

namespace QSsh {
    class SshConnectionParameters;
}

namespace Mer {
namespace Internal {

class MerHardwareDeviceWizard : public Utils::Wizard
{
    Q_OBJECT
public:
    explicit MerHardwareDeviceWizard(QWidget *parent = 0);
    ~MerHardwareDeviceWizard() override;

    MerHardwareDevice::Ptr device() const;

private:
    MerHardwareDeviceWizardSelectionPage m_selectionPage;
    RemoteLinux::GenericLinuxDeviceConfigurationWizardKeyDeploymentPage m_keyDeploymentPage;
    MerHardwareDeviceWizardPackageKeyDeploymentPage m_packageSingKeyDeploymentPage;
    MerHardwareDeviceWizardConnectionTestPage m_connectionTestPage;
    MerHardwareDeviceWizardSetupPage m_setupPage;
    RemoteLinux::GenericLinuxDeviceConfigurationWizardFinalPage m_finalPage;
    MerHardwareDevice::Ptr m_device;
};

} // Internal
} // Mer

#endif
