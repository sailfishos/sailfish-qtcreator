/****************************************************************************
**
** Copyright (C) 2020 Jolla Ltd.
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

#include "dbus.h"

#include <QDBusAbstractAdaptor>
#include <QDBusConnection>
#include <QDBusMessage>

#include <utils/qtcassert.h>

#include <sfdk/device.h>
#include <sfdk/emulator.h>
#include <sfdk/virtualmachine.h>

#include "remoteprocess.h"
#include "sdkmanager.h"
#include "sfdkglobal.h"

using namespace Sfdk;
using namespace Utils;

namespace Sfdk {

namespace {
const char DBUS_SERVICE_NAME_TEMPLATE[] = "org.sailfishos.sfdk.I%1";
const char DBUS_SFDK_ERROR[] = "org.sailfishos.sfdk.Error";
const char DBUS_CONFIGURED_DEVICE_PATH[] = "/device";
} // namespace anonymous

class AbstractAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT

public:
    AbstractAdaptor(QObject *object, const QDBusConnection &bus)
        : QDBusAbstractAdaptor(object)
        , m_bus(bus)
    {
    }

protected:
    QDBusConnection bus() const { return m_bus; }

    void replyError(const QDBusMessage &message, const QString &errorString) const
    {
        QDBusMessage error = message.createErrorReply(DBUS_SFDK_ERROR, errorString);
        bus().send(error);
    }

private:
    QDBusConnection m_bus;
};

class ConfiguredDeviceAdaptor : public AbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.sailfishos.sfdk.Device")

public:
    ConfiguredDeviceAdaptor(QObject *dummy, const QDBusConnection &bus)
        : AbstractAdaptor(dummy, bus)
    {
    }

public slots:
    bool isEmulator(const QDBusMessage &message)
    {
        return withDevice(message, [=](Device *device) {
            return device->machineType() == Device::EmulatorMachine;
        });
    }

    void prepare(const QDBusMessage &message)
    {
        withDevice(message, [=](Device *device) {
            RemoteProcess test;
            test.setProgram("/bin/true");
            test.setRunInTerminal(false);
            if (!SdkManager::prepareForRunOnDevice(*device, &test)
                    || test.exec() != EXIT_SUCCESS) {
                replyError(message, tr("Failed to prepare device"));
            }
        });
    }

private:
    template<typename Fn>
    auto withDevice(const QDBusMessage &message, Fn fn) const
        -> decltype(fn(std::declval<Device *>()))
    {
        auto default_t = Sfdk::default_t<decltype(fn(std::declval<Device *>()))>;

        QString errorString;

        Device *const device = SdkManager::configuredDevice(&errorString);
        if (!device) {
            replyError(message, errorString);
            return default_t();
        }

        return fn(device);
    }
};

class ConfiguredEmulatorDeviceAdaptor : public AbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.sailfishos.sfdk.Emulator")

public:
    ConfiguredEmulatorDeviceAdaptor(QObject *dummy, const QDBusConnection &bus)
        : AbstractAdaptor(dummy, bus)
    {
    }

public slots:
    bool hasVmFeature(const QString &feature, const QDBusMessage &message)
    {
        QTC_ASSERT(feature == "snapshots", return false); // implementation limit

        return withEmulator(message, [=](Emulator *emulator) {
            return emulator->virtualMachine()->features() & VirtualMachine::Snapshots;
        });
    }

    bool isRunning(const QDBusMessage &message)
    {
        return withEmulator(message, [=](Emulator *emulator) {
            return SdkManager::isEmulatorRunning(*emulator);
        });
    }

    void start(const QDBusMessage &message)
    {
        withEmulator(message, [=](Emulator *emulator) {
            if (!SdkManager::startEmulator(*emulator))
                replyError(message, tr("Failed to start emulator"));
        });
    }

    void stop(const QDBusMessage &message)
    {
        withEmulator(message, [=](Emulator *emulator) {
            if (!SdkManager::stopEmulator(*emulator))
                replyError(message, tr("Failed to stop emulator"));
        });
    }

    QStringList snapshots(const QDBusMessage &message)
    {
        return withEmulator(message, [=](Emulator *emulator) {
            return emulator->virtualMachine()->snapshots();
        });
    }

    void takeSnapshot(const QString &name, const QDBusMessage &message)
    {
        withEmulator(message, [=](Emulator *emulator) {
            QTC_ASSERT(emulator->virtualMachine()->isLockedDown(), return);
            bool ok;
            execAsynchronous(std::tie(ok), std::mem_fn(&VirtualMachine::takeSnapshot),
                    emulator->virtualMachine(), name);
            if (!ok)
                replyError(message, tr("Failed to take snapshot"));
        });
    }

    void restoreSnapshot(const QString &name, const QDBusMessage &message)
    {
        withEmulator(message, [=](Emulator *emulator) {
            QTC_ASSERT(emulator->virtualMachine()->isLockedDown(), return);
            bool ok;
            execAsynchronous(std::tie(ok), std::mem_fn(&VirtualMachine::restoreSnapshot),
                    emulator->virtualMachine(), name);
            if (!ok)
                replyError(message, tr("Failed to restore snapshot"));
        });
    }

    void removeSnapshot(const QString &name, const QDBusMessage &message)
    {
        withEmulator(message, [=](Emulator *emulator) {
            QTC_ASSERT(emulator->virtualMachine()->isLockedDown(), return);
            bool ok;
            execAsynchronous(std::tie(ok), std::mem_fn(&VirtualMachine::removeSnapshot),
                    emulator->virtualMachine(), name);
            if (!ok)
                replyError(message, tr("Failed to remove snapshot"));
        });
    }

    void setSharedMediaPath(const QString &path, const QDBusMessage &message)
    {
        withEmulator(message, [=](Emulator *emulator) {
            QTC_ASSERT(emulator->virtualMachine()->isLockedDown(), return);
            bool ok;
            execAsynchronous(std::tie(ok), std::mem_fn(&Emulator::setSharedMediaPath),
                    emulator, FilePath::fromString(path));
            if (!ok)
                replyError(message, tr("Failed to set shared media path"));
        });
    }

private:
    template<typename Fn>
    auto withEmulator(const QDBusMessage &message, Fn fn) const
        -> decltype(fn(std::declval<Emulator *>()))
    {
        auto default_t = Sfdk::default_t<decltype(fn(std::declval<Emulator *>()))>;

        QString errorString;

        Device *const device = SdkManager::configuredDevice(&errorString);
        if (!device) {
            replyError(message, errorString);
            return default_t();
        }

        if (device->machineType() != Device::EmulatorMachine) {
            errorString = tr("Configured device \"%1\" is not an emulator device")
                .arg(device->name());
            replyError(message, errorString);
            return default_t();
        }

        Emulator *const emulator = static_cast<EmulatorDevice *>(device)->emulator();
        QTC_ASSERT(emulator, return default_t());

        return fn(emulator);
    }
};

} // namespace Sfdk

/*!
 * \class DBusManager
 */

struct DBusManager::PrivateConstructorTag {};

QHash<QString, std::weak_ptr<DBusManager>> DBusManager::s_instances;

DBusManager::DBusManager(quint16 busPort, const QString &connectionName, const PrivateConstructorTag &)
    : m_busPort(busPort)
    , m_connectionName(connectionName)
{
    const QString address = QString("tcp:host=localhost,port=%1,family=ipv4").arg(busPort);
    QDBusConnection bus = QDBusConnection::connectToBus(address, connectionName);

    if (!bus.isConnected()) {
        qCCritical(sfdk) << "D-Bus connection failed:" << bus.lastError().name()
            << bus.lastError().message();
        return;
    }

    QTC_CHECK(bus.name() == m_connectionName);

    m_serviceName = QString(DBUS_SERVICE_NAME_TEMPLATE).arg(QCoreApplication::applicationPid());
    if (!bus.registerService(m_serviceName)) {
        qCCritical(sfdk) << "Failed to register D-Bus service using the name" << m_serviceName
            << ":" << bus.lastError().name() << bus.lastError().message();
    }

    m_configuredDeviceObject = std::make_unique<QObject>();
    new ConfiguredDeviceAdaptor(m_configuredDeviceObject.get(), bus);
    new ConfiguredEmulatorDeviceAdaptor(m_configuredDeviceObject.get(), bus);
    if (!bus.registerObject(DBUS_CONFIGURED_DEVICE_PATH, m_configuredDeviceObject.get())) {
        qCCritical(sfdk) << "Failed to register configured device on D-Bus:"
            << bus.lastError().name() << bus.lastError().message();
    }
}

DBusManager::~DBusManager()
{
    QDBusConnection::disconnectFromBus(m_connectionName);
}

DBusManager::Ptr DBusManager::get(quint16 busPort, const QString &connectionName)
{
    Ptr instance = s_instances.value(connectionName).lock();
    if (!instance) {
        instance = std::make_shared<DBusManager>(busPort, connectionName, PrivateConstructorTag{});
        s_instances.insert(connectionName, instance);
        return instance;
    }

    if (instance->m_busPort != busPort) {
        qCCritical(sfdk) << "D-Bus connection name" << connectionName
            << "already used with different port.";
        return {};
    }

    return instance;
}

#include "dbus.moc"
