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

#include "sfdkglobal.h"

#include <memory>

namespace QSsh {
class SshConnectionParameters;
}

namespace Utils {
class PortList;
}

namespace Sfdk {

class Emulator;

class DevicePrivate;
class SFDK_EXPORT Device : public QObject
{
    Q_OBJECT

public:
    enum Architecture {
        ArmArchitecture,
        X86Architecture,
    };

    enum MachineType {
        HardwareMachine,
        EmulatorMachine,
    };

    ~Device() override;

    QString id() const;

    QString name() const;
    void setName(const QString &name);

    bool isAutodetected() const;
    Architecture architecture() const;
    MachineType machineType() const;

    virtual QSsh::SshConnectionParameters sshParameters() const = 0;

    virtual Utils::PortList freePorts() const = 0;
    virtual Utils::PortList qmlLivePorts() const = 0;

signals:
    void nameChanged(const QString &name);
    void sshParametersChanged();
    void freePortsChanged();
    void qmlLivePortsChanged();

protected:
    Device(std::unique_ptr<DevicePrivate> &&dd, const QString &id, bool autodetected,
            Architecture architecture, MachineType machineType, QObject *parent);
    std::unique_ptr<DevicePrivate> d_ptr;

private:
    Q_DISABLE_COPY(Device)
    Q_DECLARE_PRIVATE(Device)
};

class HardwareDevicePrivate;
class SFDK_EXPORT HardwareDevice final : public Device
{
    Q_OBJECT

public:
    HardwareDevice(const QString &id, Architecture architecture, QObject *parent = nullptr);
    ~HardwareDevice() override;

    QSsh::SshConnectionParameters sshParameters() const final;
    void setSshParameters(const QSsh::SshConnectionParameters &sshParameters);

    Utils::PortList freePorts() const final;
    void setFreePorts(const Utils::PortList &freePorts);
    Utils::PortList qmlLivePorts() const final;
    void setQmlLivePorts(const Utils::PortList &qmlLivePorts);

private:
    Q_DISABLE_COPY(HardwareDevice)
    Q_DECLARE_PRIVATE(HardwareDevice)
};

class EmulatorDevicePrivate;
class SFDK_EXPORT EmulatorDevice final : public Device
{
    Q_OBJECT

public:
    struct PrivateConstructorTag;
    EmulatorDevice(Emulator *emulator, QObject *parent, const PrivateConstructorTag &);
    ~EmulatorDevice() override;

    Emulator *emulator() const;

    QSsh::SshConnectionParameters sshParameters() const final;

    Utils::PortList freePorts() const final;
    Utils::PortList qmlLivePorts() const final;

private:
    Q_DISABLE_COPY(EmulatorDevice)
    Q_DECLARE_PRIVATE(EmulatorDevice)
};

} // namespace Sfdk
