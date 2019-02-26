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

#include "merdevicefactory.h"

#include "merconstants.h"
#include "meremulatordevice.h"
#include "meremulatordevicewizard.h"
#include "merhardwaredevice.h"
#include "merhardwaredevicewizard.h"
#include "mericons.h"
#include "mersdkmanager.h"
#include "mersshkeydeploymentdialog.h"
#include "mersettings.h"

#include <coreplugin/messagemanager.h>
#include <coreplugin/icore.h>
#include <ssh/sshconnection.h>
#include <utils/portlist.h>
#include <utils/qtcassert.h>

#include <QFileInfo>
#include <QIcon>
#include <QInputDialog>
#include <QMessageBox>

using namespace Core;
using namespace ProjectExplorer;
using namespace QSsh;
using namespace Utils;

namespace Mer {
namespace Internal {

MerDeviceFactory *MerDeviceFactory::s_instance= 0;

MerDeviceFactory *MerDeviceFactory::instance()
{
    return s_instance;
}

MerDeviceFactory::MerDeviceFactory()
{
    QTC_CHECK(!s_instance);
    s_instance = this;
    setObjectName(QLatin1String("MerDeviceFactory"));
}

MerDeviceFactory::~MerDeviceFactory()
{
    s_instance = 0;
}

QString MerDeviceFactory::displayNameForId(Core::Id type) const
{
    if (type == Constants::MER_DEVICE_TYPE)
        return tr("Sailfish OS Device");
    return QString();
}

bool MerDeviceFactory::canCreate(Core::Id type)
{
    return type == Core::Id(Constants::MER_DEVICE_TYPE);
}

QList<Core::Id> MerDeviceFactory::availableCreationIds() const
{
    return QList<Core::Id>() << Core::Id(Constants::MER_DEVICE_TYPE);
}

QIcon MerDeviceFactory::iconForId(Core::Id type) const
{
    Q_UNUSED(type);
    static const QIcon icon =
            Utils::creatorTheme()->flag(Utils::Theme::FlatSideBarIcons)
            ? Utils::Icon::combinedIcon({Icons::MER_DEVICE_FLAT,
                                         Icons::MER_DEVICE_FLAT_SMALL})
            : Icons::MER_DEVICE_CLASSIC.icon();
    return icon;
}

IDevice::Ptr MerDeviceFactory::create(Core::Id id) const
{
    QTC_ASSERT(id == Constants::MER_DEVICE_TYPE, return IDevice::Ptr());

    QStringList choices = QStringList() << tr("Emulator") << tr("Physical device");
    bool ok;
    QString machineType = QInputDialog::getItem(ICore::dialogParent(),
            tr("Create a Sailfish OS Device"),
            tr("Add an emulator or physical device?"),
            choices, 0, false, &ok);
    if (!ok)
        return IDevice::Ptr();

    if (machineType == choices.first()) {
        QTC_ASSERT(MerSettings::deviceModels().count(), return IDevice::Ptr());

        MerEmulatorDeviceWizard wizard(ICore::dialogParent());
        if (wizard.exec() != QDialog::Accepted)
            return IDevice::Ptr();

        SshConnectionParameters sshParams;
        sshParams.setHost(QLatin1String("localhost"));
        sshParams.setUserName(wizard.userName());
        sshParams.setPort(wizard.sshPort());
        sshParams.timeout = wizard.timeout();
        sshParams.authenticationType = SshConnectionParameters::AuthenticationTypePublicKey;
        sshParams.privateKeyFile = wizard.userPrivateKey();

        //hardcoded values requested by customer;
        MerEmulatorDevice::Ptr device = MerEmulatorDevice::create(wizard.emulatorId());
        device->setVirtualMachine(wizard.emulatorVm());
        device->setMac(wizard.mac());
        device->setSubnet(QLatin1String("10.220.220"));
        device->setDisplayName(wizard.configName());
        device->setFreePorts(PortList::fromString(wizard.freePorts()));
        device->setQmlLivePorts(PortList::fromString(wizard.qmlLivePorts()));
        device->setSshParameters(sshParams);
        device->setSharedConfigPath(wizard.sharedConfigPath());
        device->setSharedSshPath(wizard.sharedSshPath());
        device->setDeviceModel(MerSettings::deviceModels().first().name());

        if (wizard.isUserNewSshKeysRquired() && !wizard.userPrivateKey().isEmpty()) {
            device->generateSshKey(wizard.userName());
        }

        if (wizard.isRootNewSshKeysRquired() && !wizard.rootPrivateKey().isEmpty()) {
            device->generateSshKey(wizard.rootName());
        }

        emit const_cast<MerDeviceFactory *>(this)->deviceCreated(device);

        return device;
    } else {
        MerHardwareDeviceWizard wizard(ICore::dialogParent());
        if (wizard.exec() != QDialog::Accepted)
            return IDevice::Ptr();

        if (wizard.isNewSshKeysRquired() && !wizard.privateKeyFilePath().isEmpty()) {

            QString privKeyPath = wizard.privateKeyFilePath();

            if (QFileInfo(privKeyPath).exists()) {
                QFile(privKeyPath).remove();
            }

            QString error;

            if (!MerSdkManager::generateSshKey(privKeyPath, error)) {
                QMessageBox::critical(ICore::dialogParent(), tr("Could not generate key."), error);
            }
        }

        {
            SshConnectionParameters sshParams;
            sshParams.setHost(wizard.hostName());
            sshParams.setUserName(wizard.userName());
            sshParams.setPort(wizard.sshPort());
            sshParams.timeout = wizard.timeout();
            sshParams.authenticationType = SshConnectionParameters::AuthenticationTypePassword;
            sshParams.setPassword(wizard.password());
            MerSshKeyDeploymentDialog dlg(ICore::dialogParent());
            dlg.setSShParameters(sshParams);
            dlg.setPublicKeyPath(wizard.publicKeyFilePath());
            if (dlg.exec() == QDialog::Rejected) {
                return IDevice::Ptr();
            }
        }

        SshConnectionParameters sshParams;
        //sshParams.options &= ~SshConnectionOptions(SshEnableStrictConformanceChecks); // For older SSH servers.
        sshParams.setHost(wizard.hostName());
        sshParams.setUserName(wizard.userName());
        sshParams.setPort(wizard.sshPort());
        sshParams.timeout = wizard.timeout();
        sshParams.authenticationType = SshConnectionParameters::AuthenticationTypePublicKey;
        sshParams.privateKeyFile = wizard.privateKeyFilePath();

        MerHardwareDevice::Ptr device = MerHardwareDevice::create(wizard.configurationName());
        device->setArchitecture(wizard.architecture());
        device->setSharedSshPath(wizard.sharedSshPath());
        device->setFreePorts(PortList::fromString(wizard.freePorts()));
        //device->setFreePorts(PortList::fromString(QLatin1String("10000-10100")));
        device->setSshParameters(sshParams);

        emit const_cast<MerDeviceFactory *>(this)->deviceCreated(device);

        return device;
    }

    return IDevice::Ptr();
}

bool MerDeviceFactory::canRestore(const QVariantMap &map) const
{
    return canCreate(IDevice::typeFromMap(map));
}

IDevice::Ptr MerDeviceFactory::restore(const QVariantMap &map) const
{
    QTC_ASSERT(canRestore(map), return IDevice::Ptr());
    if (MerDevice::workaround_machineTypeFromMap(map) == IDevice::Emulator) {
        QTC_ASSERT(MerSettings::deviceModels().count(), return IDevice::Ptr());

        const IDevice::Ptr device = MerEmulatorDevice::create();
        device->fromMap(map);

        const auto emulatorDevice = device.dynamicCast<MerEmulatorDevice>();
        if (!MerSettings::deviceModels().contains(emulatorDevice->deviceModel())) {
            const QString name = MerSettings::deviceModels().first().name();
            const QString msg = tr("Unable to find device model \"%1\"! Switching to device model \"%2\".")
                    .arg(emulatorDevice->deviceModel())
                    .arg(name);
            emulatorDevice->setDeviceModel(name);
            MessageManager::write(msg, MessageManager::Silent);
        }

        return device;
    } else {
        const IDevice::Ptr device = MerHardwareDevice::create();
        device->fromMap(map);
        return device;
    }
}

} // Internal
} // Mer
