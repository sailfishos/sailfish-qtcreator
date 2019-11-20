/****************************************************************************
**
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

#pragma once

#include "device.h"

#include <ssh/sshconnection.h>
#include <utils/portlist.h>

#include <QBasicTimer>
#include <QPointer>

namespace Sfdk {

class UserSettings;

class DevicePrivate
{
    Q_DECLARE_PUBLIC(Device)

public:
    explicit DevicePrivate(Device *q);
    virtual ~DevicePrivate();

    static DevicePrivate *get(Device *q) { return q->d_func(); }

    virtual QVariantMap toMap() const;
    virtual bool fromMap(const QVariantMap &data);

protected:
    Device *const q_ptr;

private:
    QString id;
    QString name;
    bool autodetected = false;
    Device::Architecture architecture = Device::ArmArchitecture;
    Device::MachineType machineType = Device::HardwareMachine;
    QSsh::SshConnectionParameters sshParameters;
};

class HardwareDevicePrivate : public DevicePrivate
{
    Q_DECLARE_PUBLIC(HardwareDevice)

public:
    using DevicePrivate::DevicePrivate;

    static HardwareDevicePrivate *get(HardwareDevice *q) { return q->d_func(); }

    QVariantMap toMap() const override;
    bool fromMap(const QVariantMap &data) override;

private:
    QSsh::SshConnectionParameters sshParameters;
    Utils::PortList freePorts;
    Utils::PortList qmlLivePorts;
};

class EmulatorDevicePrivate : public DevicePrivate
{
    Q_DECLARE_PUBLIC(EmulatorDevice)

public:
    using DevicePrivate::DevicePrivate;

    static EmulatorDevicePrivate *get(EmulatorDevice *q) { return q->d_func(); }

    QVariantMap toMap() const override;
    bool fromMap(const QVariantMap &data) override;

private:
    QPointer<Emulator> emulator;
};

class DeviceManager : public QObject
{
    Q_OBJECT

    class DeviceData
    {
    public:
        QString m_ip;
        QString m_sshKeyPath;
        QString m_mac;
        QString m_subNet;
        QString m_name;
        QString m_type;
        QString m_index;
        QString m_sshPort;
    };

    class EngineData
    {
    public:
        QString m_name;
        QString m_type;
        QString m_subNet;
    };

public:
    explicit DeviceManager(QObject *parent);
    ~DeviceManager() override;
    static DeviceManager *instance();

    static QList<Device *> devices();
    static Device *device(const QString &id);
    static Device *device(const Emulator &emulator);
    static int addDevice(std::unique_ptr<Device> &&device);
    static void removeDevice(const QString &id);

signals:
    void deviceAdded(int index);
    void aboutToRemoveDevice(int index);

protected:
    void timerEvent(QTimerEvent *event) override;

private:
    QVariantMap toMap() const;
    void fromMap(const QVariantMap &data);
    void enableUpdates();
    void updateOnce();
    void checkSystemSettings();
    void saveSettings(QStringList *errorStrings) const;
    void onEmulatorAdded(int index);
    void onAboutToRemoveEmulator(int index);
    void updateDevicesXml() const;
    static void writeDevicesXml(const QString &fileName, const QList<DeviceData> &devices,
            const EngineData &engine);

private:
    static DeviceManager *s_instance;
    const std::unique_ptr<UserSettings> m_userSettings;
    std::vector<std::unique_ptr<Device>> m_devices;
    QBasicTimer m_updateDevicesXmlTimer;
};

} // namespace Sfdk
