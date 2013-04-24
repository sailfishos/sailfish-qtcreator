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

#include "merdeviceconfigurationwizard.h"
#include "merdeviceconfigurationwizardsetuppages.h"
#include "merdevice.h"
#include "merconstants.h"

#include <remotelinux/linuxdevicetestdialog.h>
#include <remotelinux/linuxdevicetester.h>
#include <utils/portlist.h>
#include <utils/qtcassert.h>

using namespace ProjectExplorer;
using namespace QSsh;

namespace Mer {
namespace Internal {

namespace {
enum PageId { GeneralPageId,
              PreviousKeySetupCheckPageId,
              ReuseKeysCheckPageId,
              KeyCreationPageId,
              FinalPageId };
} // Anonymous

class MerDeviceConfigurationWizardPrivate
{
public:
    MerDeviceConfigurationWizardPrivate(Core::Id id,QWidget *parent)
        : generalPage(wizardData, parent)
        , previousKeySetupPage(parent)
        , reuseKeysCheckPage(parent)
        , keyCreationPage(wizardData, parent)
        , finalPage(wizardData, parent)
    {
        wizardData.deviceType = id;
        wizardData.machineType = id == Constants::MER_DEVICE_TYPE_I486 ? ProjectExplorer::IDevice::Emulator : ProjectExplorer::IDevice::Hardware;
    }

    WizardData wizardData;
    MerDeviceConfigWizardGeneralPage generalPage;
    MerDeviceConfigWizardPreviousKeySetupCheckPage previousKeySetupPage;
    MerDeviceConfigWizardReuseKeysCheckPage reuseKeysCheckPage;
    MerDeviceConfigWizardKeyCreationPage keyCreationPage;
    MerDeviceConfigWizardFinalPage finalPage;
};

MerDeviceConfigurationWizard::MerDeviceConfigurationWizard(Core::Id id,QWidget *parent)
    : QWizard(parent)
    , d(new MerDeviceConfigurationWizardPrivate(id,this))
{
    setWindowTitle(tr("New Mer Device Configuration Setup"));
    setPage(GeneralPageId, &d->generalPage);
    setPage(PreviousKeySetupCheckPageId, &d->previousKeySetupPage);
    setPage(ReuseKeysCheckPageId, &d->reuseKeysCheckPage);
    setPage(KeyCreationPageId, &d->keyCreationPage);
    setPage(FinalPageId, &d->finalPage);
    d->finalPage.setCommitPage(true);
}

MerDeviceConfigurationWizard::~MerDeviceConfigurationWizard()
{
    delete d;
}

IDevice::Ptr MerDeviceConfigurationWizard::device()
{
    SshConnectionParameters sshParams;
    sshParams.host = d->wizardData.hostName;
    sshParams.userName = d->wizardData.userName;
    sshParams.port = d->wizardData.sshPort;
    sshParams.timeout = 10;
    sshParams.authenticationType = d->wizardData.authType;
    if (sshParams.authenticationType == SshConnectionParameters::AuthenticationByPassword)
        sshParams.password = d->wizardData.password;
    else
        sshParams.privateKeyFile = d->wizardData.privateKeyFilePath;
    IDevice::Ptr device = MerDevice::create(d->wizardData.configName,
                                            d->wizardData.deviceType,
                                            d->wizardData.machineType,
                                            IDevice::ManuallyAdded);
    device->setFreePorts(Utils::PortList::fromString(d->wizardData.freePorts));
    device->setSshParameters(sshParams);
    // Might be called after accept.
    QWidget *parent = isVisible() ? this : static_cast<QWidget *>(0);
    RemoteLinux::LinuxDeviceTestDialog dlg(device,
                                           new RemoteLinux::GenericLinuxDeviceTester(this),
                                           parent);
    dlg.exec();
    return device;
}

int MerDeviceConfigurationWizard::nextId() const
{
    switch (currentId()) {
    case GeneralPageId:
        d->wizardData.configName = d->generalPage.configName();
        d->wizardData.hostName = d->generalPage.hostName();
        d->wizardData.sshPort = d->generalPage.sshPort();
        d->wizardData.userName = d->generalPage.userName();
        d->wizardData.authType = d->generalPage.authType();
        d->wizardData.password = d->generalPage.password();
        d->wizardData.freePorts = d->generalPage.freePorts();
        if (SshConnectionParameters::AuthenticationByPassword == d->wizardData.authType)
            return FinalPageId;
        else
            return PreviousKeySetupCheckPageId;
    case PreviousKeySetupCheckPageId:
        if (d->previousKeySetupPage.keyBasedLoginWasSetup()) {
            d->wizardData.privateKeyFilePath = d->previousKeySetupPage.privateKeyFilePath();
            return FinalPageId;
        } else {
            return ReuseKeysCheckPageId;
        }
    case ReuseKeysCheckPageId:
        if (d->reuseKeysCheckPage.reuseKeys()) {
            d->wizardData.privateKeyFilePath = d->reuseKeysCheckPage.privateKeyFilePath();
            d->wizardData.publicKeyFilePath = d->reuseKeysCheckPage.publicKeyFilePath();
            return FinalPageId;
        } else {
            return KeyCreationPageId;
        }
    case KeyCreationPageId:
        d->wizardData.privateKeyFilePath = d->keyCreationPage.privateKeyFilePath();
        d->wizardData.publicKeyFilePath = d->keyCreationPage.publicKeyFilePath();
        return FinalPageId;
    case FinalPageId:
        return -1;
    default:
        QTC_ASSERT(false, return -1);
    }
}

} // Internal
} // Mer
