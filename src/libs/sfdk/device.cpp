/****************************************************************************
**
** Copyright (C) 2019 Jolla Ltd.
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

#include "device_p.h"

#include "buildengine.h"
#include "emulator_p.h"
#include "sdk_p.h"
#include "sfdkconstants.h"
#include "usersettings_p.h"
#include "virtualmachine.h"

#include <utils/optional.h>
#include <utils/pointeralgorithm.h>

using namespace QSsh;
using namespace Utils;

namespace Sfdk {

namespace {

const char DEVICES_XML_FILE_NAME[] = "devices.xml";
const char DEVICE[] = "device";
const char DEVICES[] = "devices";
const char NAME[] = "name";
const char TYPE[] = "type";
const char TYPE_REAL[] = "real";
const char TYPE_VBOX[] = "vbox";
const char ENGINE[] = "engine";
const char IP[] = "ip";
const char SSH_PATH[] = "sshkeypath";
const char SUBNET[] = "subnet";
const char MAC[] = "mac";
const char INDEX[] = "index";
const char SSH_PORT[] = "sshport";

} // namespace anonymous

/*!
 * \class Device
 */

Device::Device(std::unique_ptr<DevicePrivate> &&dd, const QString &id, bool autodetected,
        Architecture architecture, MachineType machineType, QObject *parent)
    : QObject(parent)
    , d_ptr(std::move(dd))
{
    Q_D(Device);
    d->id = id;
    d->autodetected = autodetected;
    d->architecture = architecture;
    d->machineType = machineType;
}

Device::~Device() = default;

QString Device::id() const
{
    return d_func()->id;
}

QString Device::name() const
{
    return d_func()->name;
}

void Device::setName(const QString &name)
{
    Q_D(Device);

    if (d->name == name)
        return;
    d->name = name;
    emit nameChanged(name);
}

bool Device::isAutodetected() const
{
    return d_func()->autodetected;
}

Device::Architecture Device::architecture() const
{
    return d_func()->architecture;
}

Device::MachineType Device::machineType() const
{
    return d_func()->machineType;
}

/*!
 * \class DevicePrivate
 */

DevicePrivate::DevicePrivate(Device *q)
    : q_ptr(q)
{
}

DevicePrivate::~DevicePrivate() = default;

QVariantMap DevicePrivate::toMap() const
{
    QVariantMap data;

    data.insert(Constants::DEVICE_ID, id);
    data.insert(Constants::DEVICE_NAME, name);
    data.insert(Constants::DEVICE_AUTODETECTED, autodetected);
    data.insert(Constants::DEVICE_ARCHITECTURE, architecture);
    data.insert(Constants::DEVICE_MACHINE_TYPE, machineType);

    return data;
}

bool DevicePrivate::fromMap(const QVariantMap &data)
{
    Q_Q(Device);

    // Invariants known since construction
    QTC_ASSERT(id == data.value(Constants::DEVICE_ID).toString(), return false);
    QTC_ASSERT(autodetected == data.value(Constants::DEVICE_AUTODETECTED).toBool(), return false);
    QTC_ASSERT(architecture == static_cast<Device::Architecture>(
            data.value(Constants::DEVICE_ARCHITECTURE).toInt()), return false);
    QTC_ASSERT(machineType == static_cast<Device::MachineType>(
            data.value(Constants::DEVICE_MACHINE_TYPE).toInt()), return false);

    q->setName(data.value(Constants::DEVICE_NAME).toString());

    return true;
}

/*!
 * \class HardwareDevice
 */

HardwareDevice::HardwareDevice(const QString &id, Architecture architecture, QObject *parent)
    : Device(std::make_unique<HardwareDevicePrivate>(this), id, false, architecture,
            HardwareMachine, parent)
{
}

HardwareDevice::~HardwareDevice() = default;

QSsh::SshConnectionParameters HardwareDevice::sshParameters() const
{
    return d_func()->sshParameters;
}

void HardwareDevice::setSshParameters(const QSsh::SshConnectionParameters &sshParameters)
{
    Q_D(HardwareDevice);
    if (d->sshParameters == sshParameters)
        return;
    d->sshParameters = sshParameters;
    emit sshParametersChanged();
}

Utils::PortList HardwareDevice::freePorts() const
{
    return d_func()->freePorts;
}

void HardwareDevice::setFreePorts(const Utils::PortList &freePorts)
{
    Q_D(HardwareDevice);

    if (d->freePorts.toString() == freePorts.toString())
        return;
    d->freePorts = freePorts;
    emit freePortsChanged();
}

Utils::PortList HardwareDevice::qmlLivePorts() const
{
    return d_func()->qmlLivePorts;
}

void HardwareDevice::setQmlLivePorts(const Utils::PortList &qmlLivePorts)
{
    Q_D(HardwareDevice);

    if (d->qmlLivePorts.toString() == qmlLivePorts.toString())
        return;
    d->qmlLivePorts = qmlLivePorts;
    emit qmlLivePortsChanged();
}

/*!
 * \class HardwareDevicePrivate
 */

QVariantMap HardwareDevicePrivate::toMap() const
{
    QVariantMap data = DevicePrivate::toMap();

    data.insert(Constants::HARDWARE_DEVICE_HOST, sshParameters.host());
    data.insert(Constants::HARDWARE_DEVICE_PORT, sshParameters.port());
    data.insert(Constants::HARDWARE_DEVICE_USER_NAME, sshParameters.userName());
    data.insert(Constants::HARDWARE_DEVICE_AUTHENTICATION_TYPE, sshParameters.authenticationType);
    data.insert(Constants::HARDWARE_DEVICE_PRIVATE_KEY_FILE, sshParameters.privateKeyFile);
    data.insert(Constants::HARDWARE_DEVICE_TIMEOUT, sshParameters.timeout);
    data.insert(Constants::HARDWARE_DEVICE_HOST_KEY_CHECKING, sshParameters.hostKeyCheckingMode);

    data.insert(Constants::HARDWARE_DEVICE_FREE_PORTS, freePorts.toString());
    data.insert(Constants::HARDWARE_DEVICE_QML_LIVE_PORTS, qmlLivePorts.toString());

    return data;
}

bool HardwareDevicePrivate::fromMap(const QVariantMap &data)
{
    Q_Q(HardwareDevice);

    if (!DevicePrivate::fromMap(data))
        return false;

    SshConnectionParameters sshParameters;
    sshParameters.setHost(data.value(Constants::HARDWARE_DEVICE_HOST).toString());
    sshParameters.setPort(data.value(Constants::HARDWARE_DEVICE_PORT).toInt());
    sshParameters.setUserName(data.value(Constants::HARDWARE_DEVICE_USER_NAME).toString());
    sshParameters.authenticationType = static_cast<SshConnectionParameters::AuthenticationType>(
            data.value(Constants::HARDWARE_DEVICE_AUTHENTICATION_TYPE).toInt());
    sshParameters.privateKeyFile = data.value(Constants::HARDWARE_DEVICE_PRIVATE_KEY_FILE)
        .toString();
    sshParameters.timeout = data.value(Constants::HARDWARE_DEVICE_TIMEOUT).toInt();
    sshParameters.hostKeyCheckingMode = static_cast<QSsh::SshHostKeyCheckingMode>(
            data.value(Constants::HARDWARE_DEVICE_HOST_KEY_CHECKING).toInt());
    q->setSshParameters(sshParameters);

    q->setFreePorts(PortList::fromString(data.value(Constants::HARDWARE_DEVICE_FREE_PORTS)
                .toString()));
    q->setQmlLivePorts(PortList::fromString(data.value(Constants::HARDWARE_DEVICE_QML_LIVE_PORTS)
                .toString()));

    return true;
}

/*!
 * \class EmulatorDevice
 */

struct EmulatorDevice::PrivateConstructorTag {};

EmulatorDevice::EmulatorDevice(Emulator *emulator, QObject *parent, const PrivateConstructorTag &)
    : Device(std::make_unique<EmulatorDevicePrivate>(this), emulator->uri().toString(), true,
        X86Architecture, EmulatorMachine, parent)
{
    Q_D(EmulatorDevice);
    d->emulator = emulator;
    connect(emulator->virtualMachine(), &VirtualMachine::sshParametersChanged,
            this, &Device::sshParametersChanged);
    connect(emulator, &Emulator::freePortsChanged, this, &Device::freePortsChanged);
    connect(emulator, &Emulator::qmlLivePortsChanged, this, &Device::qmlLivePortsChanged);
}

EmulatorDevice::~EmulatorDevice() = default;

Emulator *EmulatorDevice::emulator() const
{
    return d_func()->emulator;
}

QSsh::SshConnectionParameters EmulatorDevice::sshParameters() const
{
    return d_func()->emulator->virtualMachine()->sshParameters();
}

Utils::PortList EmulatorDevice::freePorts() const
{
    return d_func()->emulator->freePorts();
}

Utils::PortList EmulatorDevice::qmlLivePorts() const
{
    return d_func()->emulator->qmlLivePorts();
}

/*!
 * \class EmulatorDevicePrivate
 */

QVariantMap EmulatorDevicePrivate::toMap() const
{
    QVariantMap data = DevicePrivate::toMap();

    data.insert(Constants::EMULATOR_DEVICE_EMULATOR_URI, emulator->uri().toString());

    return data;
}

bool EmulatorDevicePrivate::fromMap(const QVariantMap &data)
{
    if (!DevicePrivate::fromMap(data))
        return false;

    // Invariants known since construction
    QTC_ASSERT(emulator->uri() == QUrl(data.value(Constants::EMULATOR_DEVICE_EMULATOR_URI)
                .toString()), return false);

    return true;
}

/*!
 * \class DeviceManager
 */

DeviceManager *DeviceManager::s_instance = nullptr;

DeviceManager::DeviceManager(QObject *parent)
    : QObject(parent)
    , m_userSettings(std::make_unique<UserSettings>(
                Constants::DEVICES_SETTINGS_FILE_NAME,
                Constants::DEVICES_SETTINGS_DOC_TYPE, this))
{
    Q_ASSERT(!s_instance);
    s_instance = this;

    if (!SdkPrivate::useSystemSettingsOnly()) {
        // FIXME ugly
        const optional<QVariantMap> userData = m_userSettings->load();
        if (userData)
            fromMap(userData.value());
    }

    if (SdkPrivate::isVersionedSettingsEnabled()) {
        connect(SdkPrivate::instance(), &SdkPrivate::enableUpdatesRequested,
                this, &DeviceManager::enableUpdates);
    } else {
        connect(SdkPrivate::instance(), &SdkPrivate::updateOnceRequested,
                this, &DeviceManager::updateOnce);
    }

    connect(SdkPrivate::instance(), &SdkPrivate::saveSettingsRequested,
            this, &DeviceManager::saveSettings);

    connect(EmulatorManager::instance(), &EmulatorManager::emulatorAdded,
            this, &DeviceManager::onEmulatorAdded);
    connect(EmulatorManager::instance(), &EmulatorManager::aboutToRemoveEmulator,
            this, &DeviceManager::onAboutToRemoveEmulator);

    auto scheduleUpdateDeviceXmlIfPrimary = [=]() {
        if (!SdkPrivate::isVersionedSettingsEnabled() || !Sdk::isApplyingUpdates())
            m_updateDevicesXmlTimer.start(0, this);
    };
    connect(this, &DeviceManager::deviceAdded, this, scheduleUpdateDeviceXmlIfPrimary);
    connect(this, &DeviceManager::aboutToRemoveDevice, this, scheduleUpdateDeviceXmlIfPrimary);
}

DeviceManager::~DeviceManager()
{
    s_instance = nullptr;
}

DeviceManager *DeviceManager::instance()
{
    return s_instance;
}

QList<Device *> DeviceManager::devices()
{
    return Utils::toRawPointer<QList>(s_instance->m_devices);
}

Device *DeviceManager::device(const QString &id)
{
    return Utils::findOrDefault(s_instance->m_devices, Utils::equal(&Device::id, id));
}

Device *DeviceManager::device(const Emulator &emulator)
{
    return device(emulator.uri().toString());
}

int DeviceManager::addDevice(std::unique_ptr<Device> &&device)
{
    QTC_ASSERT(device, return -1);
    QTC_ASSERT(!DeviceManager::device(device->id()), return -1);

    s_instance->m_devices.emplace_back(std::move(device));
    const int index = s_instance->m_devices.size() - 1;
    emit s_instance->deviceAdded(index);
    return index;
}

void DeviceManager::removeDevice(const QString &id)
{
    int index = Utils::indexOf(s_instance->m_devices, Utils::equal(&Device::id, id));
    QTC_ASSERT(index >= 0, return);

    emit s_instance->aboutToRemoveDevice(index);
    s_instance->m_devices.erase(s_instance->m_devices.cbegin() + index);
}

void DeviceManager::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == m_updateDevicesXmlTimer.timerId()) {
        m_updateDevicesXmlTimer.stop();
        updateDevicesXml();
    } else  {
        QObject::timerEvent(event);
    }
}

QVariantMap DeviceManager::toMap() const
{
    QVariantMap data;

    int count = 0;
    for (const auto &device : m_devices) {
        const QVariantMap deviceData = DevicePrivate::get(device.get())->toMap();
        QTC_ASSERT(!deviceData.isEmpty(), return {});
        data.insert(QString::fromLatin1(Constants::DEVICES_DATA_KEY_PREFIX)
                + QString::number(count),
                deviceData);
        ++count;
    }
    data.insert(Constants::DEVICES_COUNT_KEY, count);

    return data;
}

void DeviceManager::fromMap(const QVariantMap &data)
{
    const int newCount = data.value(Constants::DEVICES_COUNT_KEY, 0).toInt();
    QMap<QString, QVariantMap> newDevicesData;
    for (int i = 0; i < newCount; ++i) {
        const QString key = QString::fromLatin1(Constants::DEVICES_DATA_KEY_PREFIX)
            + QString::number(i);
        QTC_ASSERT(data.contains(key), return);

        const QVariantMap deviceData = data.value(key).toMap();
        const QString id = deviceData.value(Constants::DEVICE_ID).toString();
        QTC_ASSERT(!id.isEmpty(), return);

        newDevicesData.insert(id, deviceData);
    }

    // Remove devices missing from the updated configuration.
    // Index the preserved ones by ID
    QMap<QString, Device *> existingDevices;
    for (auto it = m_devices.cbegin(); it != m_devices.cend(); ) {
        if (!newDevicesData.contains(it->get()->id())) {
            emit aboutToRemoveDevice(it - m_devices.cbegin());
            it = m_devices.erase(it);
        } else {
            existingDevices.insert(it->get()->id(), it->get());
            ++it;
        }
    }

    // Update existing/add new devices
    for (const QString &id : newDevicesData.keys()) {
        const QVariantMap deviceData = newDevicesData.value(id);
        Device *device = existingDevices.value(id);
        std::unique_ptr<Device> newDevice;
        if (!device) {
            const auto machineType = static_cast<Device::MachineType>(
                    deviceData.value(Constants::DEVICE_MACHINE_TYPE).toInt());
            switch (machineType) {
            case Device::HardwareMachine:
                {
                    const QString id = deviceData.value(Constants::DEVICE_ID).toString();
                    const auto architecture = static_cast<Device::Architecture>(
                            deviceData.value(Constants::DEVICE_ARCHITECTURE).toInt());
                    newDevice = std::make_unique<HardwareDevice>(id, architecture, this);
                    break;
                }
            case Device::EmulatorMachine:
                {
                    const QUrl emulatorUri =
                        deviceData.value(Constants::EMULATOR_DEVICE_EMULATOR_URI).toUrl();
                    Emulator *const emulator = EmulatorManager::emulator(emulatorUri);
                    QTC_ASSERT(emulator, return); // inter-file inconsistency
                    newDevice = std::make_unique<EmulatorDevice>(emulator, this,
                            EmulatorDevice::PrivateConstructorTag{});
                    break;
                }
            }
            device = newDevice.get();
        }

        const bool ok = DevicePrivate::get(device)->fromMap(deviceData);
        QTC_ASSERT(ok, return);

        if (newDevice) {
            m_devices.emplace_back(std::move(newDevice));
            emit deviceAdded(m_devices.size() - 1);
        }
    }
}

void DeviceManager::enableUpdates()
{
    QTC_ASSERT(SdkPrivate::isVersionedSettingsEnabled(), return);

    qCDebug(Log::device) << "Enabling updates";

    connect(m_userSettings.get(), &UserSettings::updated,
            this, [=](const QVariantMap &data) { fromMap(data); });
    m_userSettings->enableUpdates();
}

void DeviceManager::updateOnce()
{
    QTC_ASSERT(!SdkPrivate::isVersionedSettingsEnabled(), return);
    // nothing to do
}

void DeviceManager::saveSettings(QStringList *errorStrings) const
{
    QString errorString;
    const bool ok = m_userSettings->save(toMap(), &errorString);
    if (!ok)
        errorStrings->append(errorString);
}

void DeviceManager::onEmulatorAdded(int index)
{
    Emulator *const emulator = EmulatorManager::emulators().at(index);
    auto device = std::make_unique<EmulatorDevice>(emulator, this,
            EmulatorDevice::PrivateConstructorTag{});
    device->setName(emulator->virtualMachine()->name());
    addDevice(std::move(device));
}

void DeviceManager::onAboutToRemoveEmulator(int index)
{
    Emulator *const emulator = EmulatorManager::emulators().at(index);
    const int deviceIndex = Utils::indexOf(s_instance->m_devices,
            [=](const std::unique_ptr<Device> &device) {
        return device->machineType() == Device::EmulatorMachine
            && static_cast<EmulatorDevice *>(device.get())->emulator() == emulator;
    });
    QTC_ASSERT(deviceIndex >= 0, return);
    emit aboutToRemoveDevice(deviceIndex);
    m_devices.erase(m_devices.cbegin() + deviceIndex);
}

// TODO multiple build engines
void DeviceManager::updateDevicesXml() const
{
    QList<DeviceData> devices;
    QList<FileName> emulatorConfigPaths;

    for (const std::unique_ptr<Device> &device : m_devices) {
        DeviceData xmlData;
        if (device->machineType() == Device::HardwareMachine) {
            xmlData.m_ip = device->sshParameters().host();
            xmlData.m_name = device->name();
            xmlData.m_type = TYPE_REAL;
            xmlData.m_sshPort.setNum(device->sshParameters().port());
            const FileName sharedConfigPath = Sdk::buildEngines().first()->sharedConfigPath();
            if (!sharedConfigPath.isEmpty()) {
                xmlData.m_sshKeyPath =
                    FileName::fromString(device->sshParameters().privateKeyFile).parentDir()
                    .relativeChildPath(sharedConfigPath).toString();
            }
        } else {
            EmulatorDevice *const emulatorDevice = static_cast<EmulatorDevice *>(device.get());
            QString mac = EmulatorPrivate::get(emulatorDevice->emulator())->mac();
            QTC_CHECK(!mac.isEmpty());
            if (!mac.isEmpty())
                xmlData.m_index = mac.at(mac.count() - 1);
            xmlData.m_subNet = EmulatorPrivate::get(emulatorDevice->emulator())->subnet();
            xmlData.m_name = device->name();
            xmlData.m_mac = mac;
            xmlData.m_type = TYPE_VBOX;
            const FileName sharedConfigPath = Sdk::buildEngines().first()->sharedConfigPath();
            if (!sharedConfigPath.isEmpty()) {
                xmlData.m_sshKeyPath =
                    FileName::fromString(device->sshParameters().privateKeyFile).parentDir()
                    .relativeChildPath(sharedConfigPath).toString();
                emulatorConfigPaths << emulatorDevice->emulator()->sharedConfigPath();
            }
        }
        devices << xmlData;
    }

    for (BuildEngine *const engine : Sdk::buildEngines()) {
        EngineData xmlData;
        xmlData.m_name = engine->name();
        xmlData.m_type =  TYPE_VBOX;
        xmlData.m_subNet = Constants::VIRTUAL_BOX_VIRTUAL_SUBNET;

        const QString file =
            engine->sharedConfigPath().appendPath(DEVICES_XML_FILE_NAME).toString();
        if (!file.isEmpty())
            writeDevicesXml(file, devices, xmlData);

        // The emulators only seek their own data in the XML
        for (const FileName &emulatorConfigPath : emulatorConfigPaths) {
            const QString file =
                FileName(emulatorConfigPath).appendPath(DEVICES_XML_FILE_NAME).toString();
            writeDevicesXml(file, devices, xmlData);
        }
    }
}

// Example output:
//
//<devices>
// <engine name="Sailfish OS Build Engine" type="vbox">
//  <subnet>10.220.220</subnet
// </engine>
// <device name="Nemo N9" type="real">
//  <ip>192.168.0.12</ip>
//  <sshkeypath></sshkeypath>
// </device>
// <device name="Sailfish OS Emulator" type="vbox">
//  <index>2</index>
//  <subnet>10.220.220</subnet>
//  <sshkeypath></sshkeypath>
//  <mac>08:00:27:7C:A1:AF</mac>
// </device>
//</devices>
void DeviceManager::writeDevicesXml(const QString &fileName, const QList<DeviceData> &devices,
        const EngineData &engine)
{
    QByteArray data;
    QXmlStreamWriter writer(&data);
    writer.setAutoFormatting(true);
    writer.writeStartDocument();
    writer.writeStartElement(DEVICES);
    writer.writeStartElement(ENGINE);
    writer.writeAttribute(NAME, engine.m_name);
    writer.writeAttribute(TYPE, engine.m_type);
    writer.writeStartElement(SUBNET);
    writer.writeCharacters(engine.m_subNet);
    writer.writeEndElement(); // subnet
    writer.writeEndElement(); // engine
    for (const DeviceData &data : devices) {
        writer.writeStartElement(DEVICE);
        writer.writeAttribute(NAME, data.m_name);
        writer.writeAttribute(TYPE, data.m_type);
        if (!data.m_index.isEmpty()) {
            writer.writeStartElement(INDEX);
            writer.writeCharacters(data.m_index);
            writer.writeEndElement(); // index
        }
        if (!data.m_ip.isEmpty()) {
            writer.writeStartElement(IP);
            writer.writeCharacters(data.m_ip);
            writer.writeEndElement(); // ip
        }
        if (!data.m_subNet.isEmpty()) {
            writer.writeStartElement(SUBNET);
            writer.writeCharacters(data.m_subNet);
            writer.writeEndElement(); // subnet
        }
        if (!data.m_sshKeyPath.isEmpty()) {
            writer.writeStartElement(SSH_PATH);
            writer.writeCharacters(data.m_sshKeyPath);
            writer.writeEndElement(); // ssh
        }
        if (!data.m_mac.isEmpty()) {
            writer.writeStartElement(MAC);
            writer.writeCharacters(data.m_mac);
            writer.writeEndElement(); // mac
        }
        if (!data.m_sshPort.isEmpty()) {
            writer.writeStartElement(SSH_PORT);
            writer.writeCharacters(data.m_sshPort);
            writer.writeEndElement(); // sshport
        }
        writer.writeEndElement(); // device
    }
    {
        // This is a hack. This hack allows external modifications to the xml file for devices
        // Only devices that are of type=vbox are overwritten.
        // REMOVE THESE LINES
        // STARTS HERE
        FileReader fileReader;
        if (fileReader.fetch(fileName)) {
            QXmlStreamReader xmlReader(fileReader.data());
            while (!xmlReader.atEnd()) {
                xmlReader.readNext();
                if (xmlReader.isStartElement() && xmlReader.name() == DEVICE) {
                    QXmlStreamAttributes attributes = xmlReader.attributes();
                    if (attributes.value(TYPE) == "custom") {
                        writer.writeStartElement(DEVICE);
                        writer.writeAttributes(attributes);
                        while (xmlReader.readNext()) {
                            if (xmlReader.isStartElement()) {
                                writer.writeStartElement(xmlReader.name().toString());
                                attributes = xmlReader.attributes();
                                writer.writeAttributes(attributes);
                            } else if (xmlReader.isEndElement()) {
                                writer.writeEndElement();
                                if (xmlReader.name() == DEVICE)
                                    break;
                            } else if (xmlReader.isCharacters()) {
                                writer.writeCharacters(xmlReader.text().toString());
                            }
                        }
                    }
                }
            }
        }
        // ENDS HERE
    }
    writer.writeEndElement(); // devices
    writer.writeEndDocument();

    FileSaver fileSaver(fileName, QIODevice::WriteOnly);
    fileSaver.write(data);
    if (fileSaver.finalize())
        qCDebug(Log::device) << "Updated" << fileName;
    else
        qCWarning(Log::device) << "Error updating" << fileName << fileSaver.errorString();
}

} // namespace Sfdk
