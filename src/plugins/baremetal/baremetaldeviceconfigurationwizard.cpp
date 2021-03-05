/****************************************************************************
**
** Copyright (C) 2016 Tim Sander <tim@krieglstein.org>
** Copyright (C) 2016 Denis Shienkov <denis.shienkov@gmail.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "baremetalconstants.h"
#include "baremetaldevice.h"
#include "baremetaldeviceconfigurationwizard.h"
#include "baremetaldeviceconfigurationwizardpages.h"

namespace BareMetal {
namespace Internal {

enum PageId { SetupPageId };

// BareMetalDeviceConfigurationWizard

BareMetalDeviceConfigurationWizard::BareMetalDeviceConfigurationWizard(QWidget *parent) :
   Utils::Wizard(parent),
   m_setupPage(new BareMetalDeviceConfigurationWizardSetupPage(this))
{
    setWindowTitle(tr("New Bare Metal Device Configuration Setup"));
    setPage(SetupPageId, m_setupPage);
    m_setupPage->setCommitPage(true);
}

ProjectExplorer::IDevice::Ptr BareMetalDeviceConfigurationWizard::device() const
{
    const auto dev = BareMetalDevice::create();
    dev->setupId(ProjectExplorer::IDevice::ManuallyAdded, Utils::Id());
    dev->setDisplayName(m_setupPage->configurationName());
    dev->setType(Constants::BareMetalOsType);
    dev->setMachineType(ProjectExplorer::IDevice::Hardware);
    dev->setDebugServerProviderId(m_setupPage->debugServerProviderId());
    return dev;
}

} // namespace Internal
} // namespace BareMetal
