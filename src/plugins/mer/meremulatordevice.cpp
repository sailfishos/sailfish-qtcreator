/****************************************************************************
**
** Copyright (C) 2012-2019 Jolla Ltd.
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

#include "meremulatordevice.h"

#include "merconstants.h"
#include "merdevicefactory.h"
#include "meremulatordevicetester.h"
#include "meremulatoroptionspage.h"
#include "merplugin.h"
#include "mersdkmanager.h"
#include "mersettings.h"
#include "mervmconnectionui.h"
#include "meremulatormodedialog.h"

#include <sfdk/buildengine.h>
#include <sfdk/device.h>
#include <sfdk/emulator.h>
#include <sfdk/sdk.h>
#include <sfdk/sfdkconstants.h>
#include <sfdk/virtualmachine.h>

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <ssh/sshconnection.h>
#include <utils/persistentsettings.h>

#include <QDir>
#include <QMessageBox>
#include <QProgressDialog>
#include <QPushButton>
#include <QTextStream>
#include <QTimer>

using namespace Core;
using namespace ExtensionSystem;
using namespace ProjectExplorer;
using namespace QSsh;
using namespace Sfdk;
using namespace Utils;

namespace Mer {

using namespace Internal;
using namespace Constants;

namespace {
const char EMULATOR_DEVICE_ID_SUFFIX[] = "-emulator";
} // namespace anonymous

IDevice::Ptr MerEmulatorDevice::clone() const
{
    IDevice::Ptr device = create();
    device->fromMap(toMap());
    return device;
}

MerEmulatorDevice::MerEmulatorDevice()
{
    setMachineType(IDevice::Emulator);
    setArchitecture(Abi::X86Architecture);
    setWordWidth(32);
    init();
}

MerEmulatorDevice::~MerEmulatorDevice()
{
}

IDeviceWidget *MerEmulatorDevice::createWidget()
{
    return nullptr;
}

void MerEmulatorDevice::init()
{
    auto addAction = [this](const QString &displayName,
            const std::function<void(const MerEmulatorDevice::Ptr &, QWidget *)> &handler) {
        auto wrapped = [handler](const IDevice::Ptr &device, QWidget *parent) {
            handler(device.staticCast<MerEmulatorDevice>(), parent);
        };
        addDeviceAction({displayName, wrapped});
    };

    addAction(tr("Start Emulator"), [](const MerEmulatorDevice::Ptr &device, QWidget *parent) {
        Q_UNUSED(parent);
        if (device->emulator()->virtualMachine()->isStateChangePending()) {
            MerVmConnectionUi::informStateChangePending();
            return;
        }
        device->emulator()->virtualMachine()->connectTo(VirtualMachine::NoConnectOption,
                Sdk::instance(), IgnoreAsynchronousReturn<bool>);
    });

    addAction(tr("Stop Emulator"), [](const MerEmulatorDevice::Ptr &device, QWidget *parent) {
        Q_UNUSED(parent);
        if (device->emulator()->virtualMachine()->isStateChangePending()) {
            MerVmConnectionUi::informStateChangePending();
            return;
        }
        device->emulator()->virtualMachine()->disconnectFrom(
                Sdk::instance(), IgnoreAsynchronousReturn<bool>);
    });

    addAction(tr("Configure Emulator..."), [](const MerEmulatorDevice::Ptr &device, QWidget *parent) {
        Q_UNUSED(parent);
        MerPlugin::emulatorOptionsPage()->setEmulator(device->emulator()->uri());
        QTimer::singleShot(0, ICore::instance(),
                []() { ICore::showOptionsDialog(Constants::MER_EMULATOR_OPTIONS_ID); });
    });
}

DeviceTester *MerEmulatorDevice::createDeviceTester() const
{
    return new MerEmulatorDeviceTester;
}

void MerEmulatorDevice::fromMap(const QVariantMap &map)
{
    MerDevice::fromMap(map);

    const QString sdkId = toSdkId(id());
    Device *const sdkDevice = Sdk::device(sdkId);
    QTC_ASSERT(sdkDevice, return);
    m_sdkDevice = qobject_cast<EmulatorDevice *>(sdkDevice);
    QTC_ASSERT(m_sdkDevice, return);
}

Sfdk::EmulatorDevice *MerEmulatorDevice::sdkDevice() const
{
    return m_sdkDevice;
}

void MerEmulatorDevice::setSdkDevice(Sfdk::EmulatorDevice *sdkDevice)
{
    QTC_CHECK(sdkDevice);
    QTC_CHECK(!m_sdkDevice);
    m_sdkDevice = sdkDevice;
}

Sfdk::Emulator *MerEmulatorDevice::emulator() const
{
    return m_sdkDevice->emulator();
}

void MerEmulatorDevice::doFactoryReset(Sfdk::Emulator *emulator, QWidget *parent)
{
    QProgressDialog progress(tr("Restoring '%1' to factory state...").arg(emulator->name()),
            tr("Abort"), 0, 0, parent);
    progress.setMinimumDuration(2000);
    progress.setWindowModality(Qt::WindowModal);

    bool ok;
    execAsynchronous(std::tie(ok), std::mem_fn(&VirtualMachine::lockDown),
            emulator->virtualMachine(), true);
    if (!ok) {
        progress.cancel();
        QMessageBox::warning(parent, tr("Failed"),
                tr("Failed to close the virtual machine. Factory state cannot be restored."));
        return;
    }

    execAsynchronous(std::tie(ok), std::mem_fn(&VirtualMachine::restoreSnapshot),
            emulator->virtualMachine(), emulator->factorySnapshot());
    if (!ok) {
        emulator->virtualMachine()->lockDown(false, emulator, IgnoreAsynchronousReturn<bool>);
        progress.cancel();
        QMessageBox::warning(parent, tr("Failed"), tr("Failed to restore factory state."));
        return;
    }

    emulator->virtualMachine()->lockDown(false, emulator, IgnoreAsynchronousReturn<bool>);

    progress.cancel();

    QMessageBox::information(parent, tr("Factory state restored"),
            tr("Successfully restored '%1' to the factory state").arg(emulator->name()));
}

Utils::Id MerEmulatorDevice::idFor(const Sfdk::EmulatorDevice &sdkDevice)
{
    QTC_CHECK(sdkDevice.id() == sdkDevice.emulator()->uri().toString());
    return idFor(*sdkDevice.emulator());
}

Utils::Id MerEmulatorDevice::idFor(const Sfdk::Emulator &emulator)
{
    // HACK: We know we do not have other than VirtualBox based emulators
    QTC_CHECK(emulator.uri().path() == "VirtualBox");
    QTC_CHECK(emulator.uri().fragment() == emulator.name());
    return Utils::Id::fromString(emulator.name() + EMULATOR_DEVICE_ID_SUFFIX);
}

QString MerEmulatorDevice::toSdkId(const Utils::Id &id)
{
    // HACK
    QTC_CHECK(id.toString().endsWith(EMULATOR_DEVICE_ID_SUFFIX));
    const QString emulatorName = id.toString()
        .chopped(QString(EMULATOR_DEVICE_ID_SUFFIX).length());
    QUrl emulatorUri;
    emulatorUri.setScheme(Sfdk::Constants::VIRTUAL_MACHINE_URI_SCHEME);
    emulatorUri.setPath("VirtualBox");
    emulatorUri.setFragment(emulatorName);
    return emulatorUri.toString();
}

/*!
 * \class MerEmulatorDeviceManager
 */

MerEmulatorDeviceManager *MerEmulatorDeviceManager::s_instance = 0;

MerEmulatorDeviceManager::MerEmulatorDeviceManager(QObject *parent)
    : QObject(parent)
{
    s_instance = this;

    for (Device *const sdkDevice : Sdk::devices()) {
        if (auto *const sdkEmulatorDevice = qobject_cast<EmulatorDevice *>(sdkDevice))
            startWatching(sdkEmulatorDevice);
    }

    connect(Sdk::instance(), &Sdk::deviceAdded,
            this, &MerEmulatorDeviceManager::onSdkDeviceAdded);
    connect(Sdk::instance(), &Sdk::aboutToRemoveDevice,
            this, &MerEmulatorDeviceManager::onSdkAboutToRemoveDevice);
}

MerEmulatorDeviceManager *MerEmulatorDeviceManager::instance()
{
    QTC_CHECK(s_instance);
    return s_instance;
}

MerEmulatorDeviceManager::~MerEmulatorDeviceManager()
{
    s_instance = 0;
}

void MerEmulatorDeviceManager::onSdkDeviceAdded(int index)
{
    auto *const sdkDevice = qobject_cast<EmulatorDevice *>(Sdk::devices().at(index));
    if (!sdkDevice)
        return;

    const Utils::Id id = MerEmulatorDevice::idFor(*sdkDevice);
    QTC_ASSERT(!DeviceManager::instance()->find(id), return);

    const MerEmulatorDevice::Ptr device = MerEmulatorDevice::create();
    device->setSdkDevice(sdkDevice);
    device->setupId(IDevice::AutoDetected, id);
    device->setDisplayName(sdkDevice->name());
    device->setSshParameters(sdkDevice->sshParameters());
    device->setFreePorts(sdkDevice->freePorts());
    device->setQmlLivePorts(sdkDevice->qmlLivePorts());
    startWatching(sdkDevice);

    DeviceManager::instance()->addDevice(device);
}

void MerEmulatorDeviceManager::onSdkAboutToRemoveDevice(int index)
{
    auto *const sdkDevice = qobject_cast<EmulatorDevice *>(Sdk::devices().at(index));
    if (!sdkDevice)
        return;

    stopWatching(sdkDevice);

    const Utils::Id id = MerEmulatorDevice::idFor(*sdkDevice);
    QTC_ASSERT(DeviceManager::instance()->find(id), return);

    DeviceManager::instance()->removeDevice(id);
}

void MerEmulatorDeviceManager::startWatching(Sfdk::EmulatorDevice *sdkDevice)
{
    auto withDevice = [=](const std::function<void(MerEmulatorDevice *device)> &function) {
        return [=]() {
            const auto manager = DeviceManager::instance();
            const IDevice::ConstPtr device = manager->find(MerEmulatorDevice::idFor(*sdkDevice));
            QTC_ASSERT(device, return);
            QTC_ASSERT(device.dynamicCast<const MerEmulatorDevice>(), return);
            const IDevice::Ptr clone = device->clone();

            function(static_cast<MerEmulatorDevice *>(clone.data()));

            manager->addDevice(clone);
        };
    };

    connect(sdkDevice, &EmulatorDevice::nameChanged,
            this, withDevice([](MerEmulatorDevice *device) {
        device->setDisplayName(device->sdkDevice()->name());
    }));
    connect(sdkDevice, &EmulatorDevice::sshParametersChanged,
            this, withDevice([](MerEmulatorDevice *device) {
        device->setSshParameters(device->sdkDevice()->sshParameters());
    }));
    connect(sdkDevice, &EmulatorDevice::freePortsChanged,
            this, withDevice([](MerEmulatorDevice *device) {
        device->setFreePorts(device->sdkDevice()->freePorts());
    }));
    connect(sdkDevice, &EmulatorDevice::qmlLivePortsChanged,
            this, withDevice([](MerEmulatorDevice *device) {
        device->setQmlLivePorts(device->sdkDevice()->qmlLivePorts());
    }));
}

void MerEmulatorDeviceManager::stopWatching(Sfdk::EmulatorDevice *sdkDevice)
{
    sdkDevice->disconnect(this);
}

#include "meremulatordevice.moc"

}
