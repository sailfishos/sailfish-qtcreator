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

#include <sfdk/device.h>
#include <sfdk/sdk.h>
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
using namespace Sfdk;
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

Core::Id MerHardwareDevice::idFor(const Sfdk::HardwareDevice &sdkDevice)
{
    return Core::Id::fromString(sdkDevice.id());
}

QString MerHardwareDevice::toSdkId(const Core::Id &id)
{
    return id.toString();
}

/*!
 * \class MerHardwareDeviceManager
 */

template<typename T>
T architecture_cast(int a)
{
    static_assert(Abi::ArmArchitecture == static_cast<int>(Device::ArmArchitecture),
            "Abi::Architecture / Device::Architecture mismatch");
    static_assert(Abi::X86Architecture == static_cast<int>(Device::X86Architecture),
            "Abi::Architecture / Device::Architecture mismatch");
    QTC_ASSERT(a >= 0 && a <= Device::X86Architecture, return {});
    return static_cast<T>(a);
}

MerHardwareDeviceManager *MerHardwareDeviceManager::s_instance = nullptr;

MerHardwareDeviceManager::MerHardwareDeviceManager(QObject *parent)
    : QObject(parent)
{
    s_instance = this;

    for (Device *const sdkDevice : Sdk::devices()) {
        if (auto *const sdkHwDevice = qobject_cast<HardwareDevice *>(sdkDevice))
            startWatching(sdkHwDevice);
    }

    connect(Sdk::instance(), &Sdk::deviceAdded,
            this, &MerHardwareDeviceManager::onSdkDeviceAdded);
    connect(Sdk::instance(), &Sdk::aboutToRemoveDevice,
            this, &MerHardwareDeviceManager::onSdkAboutToRemoveDevice);

    connect(DeviceManager::instance(), &DeviceManager::deviceAdded,
            this, &MerHardwareDeviceManager::onDeviceAddedOrUpdated);
    connect(DeviceManager::instance(), &DeviceManager::deviceUpdated,
            this, &MerHardwareDeviceManager::onDeviceAddedOrUpdated);
    connect(DeviceManager::instance(), &DeviceManager::deviceRemoved,
            this, &MerHardwareDeviceManager::onDeviceRemoved);
    connect(DeviceManager::instance(), &DeviceManager::deviceListReplaced,
            this, &MerHardwareDeviceManager::onDeviceListReplaced);
}

MerHardwareDeviceManager *MerHardwareDeviceManager::instance()
{
    QTC_CHECK(s_instance);
    return s_instance;
}

MerHardwareDeviceManager::~MerHardwareDeviceManager()
{
    s_instance = nullptr;
}

void MerHardwareDeviceManager::onSdkDeviceAdded(int index)
{
    auto *const sdkDevice = qobject_cast<HardwareDevice *>(Sdk::devices().at(index));
    if (!sdkDevice)
        return;

    const Core::Id id = MerHardwareDevice::idFor(*sdkDevice);
    if (DeviceManager::instance()->find(id))
        return; // device addition initiated from onDeviceAddedOrUpdated

    MerHardwareDevice::Ptr device = MerHardwareDevice::create();
    const IDevice::Origin origin = sdkDevice->isAutodetected()
        ? IDevice::AutoDetected
        : IDevice::ManuallyAdded;
    device->setupId(origin, id);
    device->setDisplayName(sdkDevice->name());
    device->setArchitecture(architecture_cast<Abi::Architecture>(sdkDevice->architecture()));
    device->setFreePorts(sdkDevice->freePorts());
    device->setQmlLivePorts(sdkDevice->qmlLivePorts());
    device->setSshParameters(sdkDevice->sshParameters());
    startWatching(sdkDevice);

    DeviceManager::instance()->addDevice(device);
}

void MerHardwareDeviceManager::onSdkAboutToRemoveDevice(int index)
{
    auto *const sdkDevice = qobject_cast<HardwareDevice *>(Sdk::devices().at(index));
    if (!sdkDevice)
        return;

    stopWatching(sdkDevice);

    const Core::Id id = MerHardwareDevice::idFor(*sdkDevice);
    if (!DeviceManager::instance()->find(id))
        return; // device removal initiated from onDeviceRemoved

    m_removingDeviceId = id;
    DeviceManager::instance()->removeDevice(id);
    m_removingDeviceId = Core::Id();
}

void MerHardwareDeviceManager::startWatching(Sfdk::HardwareDevice *sdkDevice)
{
    auto withDevice = [=](const std::function<bool(MerHardwareDevice *, HardwareDevice *)> &function) {
        return [=]() {
            const auto manager = DeviceManager::instance();
            const IDevice::ConstPtr device = manager->find(MerHardwareDevice::idFor(*sdkDevice));
            QTC_ASSERT(device, return);

            QTC_ASSERT(device.dynamicCast<const MerHardwareDevice>(), return);
            const IDevice::Ptr clone = device->clone();
            if (function(static_cast<MerHardwareDevice *>(clone.data()), sdkDevice))
                manager->addDevice(clone);
        };
    };

    connect(sdkDevice, &HardwareDevice::nameChanged,
            this, withDevice([](MerHardwareDevice *device, HardwareDevice *sdkDevice) {
        if (device->displayName() == sdkDevice->name())
            return false;
        device->setDisplayName(sdkDevice->name());
        return true;
    }));
    connect(sdkDevice, &HardwareDevice::sshParametersChanged,
            this, withDevice([](MerHardwareDevice *device, HardwareDevice *sdkDevice) {
        if (device->sshParameters() == sdkDevice->sshParameters())
            return false;
        device->setSshParameters(sdkDevice->sshParameters());
        return true;
    }));
    connect(sdkDevice, &HardwareDevice::freePortsChanged,
            this, withDevice([](MerHardwareDevice *device, HardwareDevice *sdkDevice) {
        if (device->freePorts().toString() == sdkDevice->freePorts().toString())
            return false;
        device->setFreePorts(sdkDevice->freePorts());
        return true;
    }));
    connect(sdkDevice, &HardwareDevice::qmlLivePortsChanged,
            this, withDevice([](MerHardwareDevice *device, HardwareDevice *sdkDevice) {
        if (device->qmlLivePorts().toString() == sdkDevice->qmlLivePorts().toString())
            return false;
        device->setQmlLivePorts(sdkDevice->qmlLivePorts());
        return true;
    }));
}

void MerHardwareDeviceManager::stopWatching(Sfdk::HardwareDevice *sdkDevice)
{
    sdkDevice->disconnect(this);
}

void MerHardwareDeviceManager::onDeviceAddedOrUpdated(Core::Id id)
{
    const IDevice::ConstPtr device = DeviceManager::instance()->find(id);
    QTC_ASSERT(device, return);
    const auto hwDevice = device.dynamicCast<const MerHardwareDevice>();
    if (!hwDevice)
        return;

    const QString sdkId = MerHardwareDevice::toSdkId(id);
    const auto sdkArch = architecture_cast<Device::Architecture>(hwDevice->architecture());

    Device *const sdkDevice = Sdk::device(sdkId);
    auto *sdkHwDevice = qobject_cast<HardwareDevice *>(sdkDevice);
    QTC_ASSERT(!sdkDevice || sdkHwDevice, return);

    std::unique_ptr<HardwareDevice> newSdkDevice;
    if (!sdkHwDevice) {
        newSdkDevice = std::make_unique<HardwareDevice>(sdkId, sdkArch);
        sdkHwDevice = newSdkDevice.get();
    }

    QTC_CHECK(sdkHwDevice->architecture() == sdkArch);
    sdkHwDevice->setName(hwDevice->displayName());
    sdkHwDevice->setSshParameters(hwDevice->sshParameters());
    sdkHwDevice->setFreePorts(hwDevice->freePorts());
    sdkHwDevice->setQmlLivePorts(hwDevice->qmlLivePorts());

    if (newSdkDevice)
        Sdk::addDevice(std::move(newSdkDevice));
}

void MerHardwareDeviceManager::onDeviceRemoved(Core::Id id)
{
    if (id == m_removingDeviceId)
        return;

    const QString sdkId = MerHardwareDevice::toSdkId(id);
    if (Sdk::device(sdkId))
        Sdk::removeDevice(sdkId);
}

void MerHardwareDeviceManager::onDeviceListReplaced()
{
    for (int i = 0; i < DeviceManager::instance()->deviceCount(); ++i) {
        const IDevice::ConstPtr device = DeviceManager::instance()->deviceAt(i);
        onDeviceAddedOrUpdated(device->id());
    }

    for (Device *const sdkDevice : Sdk::devices()) {
        auto *const sdkHwDevice = qobject_cast<HardwareDevice *>(sdkDevice);
        if (!sdkHwDevice)
            continue;
        const Core::Id id = MerHardwareDevice::idFor(*sdkHwDevice);
        if (!DeviceManager::instance()->find(id))
            onDeviceRemoved(id);
    }
}

} // Internal
} // Mer
