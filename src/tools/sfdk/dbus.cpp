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

#include "remoteprocess.h"
#include "sdkmanager.h"
#include "sfdkglobal.h"

using namespace Sfdk;

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
    bool prepare(const QDBusMessage &message)
    {
        QString errorString;

        Device *const device = SdkManager::configuredDevice(&errorString);
        if (!device) {
            QDBusMessage error = message.createErrorReply(DBUS_SFDK_ERROR, errorString);
            bus().send(error);
            return false;
        }

        RemoteProcess test;
        test.setProgram("/bin/true");
        test.setRunInTerminal(false);
        if (!SdkManager::prepareForRunOnDevice(*device, &test))
            return false;

        return test.exec() == EXIT_SUCCESS;
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
