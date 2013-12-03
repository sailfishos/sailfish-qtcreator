/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "adddeviceoperation.h"
#include "findkeyoperation.h"
#include "getoperation.h"
#include "addkeysoperation.h"
#include "rmkeysoperation.h"
#include "findvalueoperation.h"
#include "../../plugins/mer/merconstants.h"
#include <iostream>

// copy pasted from idevice.cpp
const char DisplayNameKey[] = "Name";
const char TypeKey[] = "OsType";
const char IdKey[] = "InternalId";
const char OriginKey[] = "Origin";
const char SdkProvidedKey[] = "SdkProvided";
const char MachineTypeKey[] = "Type";
const char DeviceManagerKey[] = "DeviceManager";
const char DeviceListKey[] = "DeviceList";
const char VersionKey[] = "Version";

// Connection
const char HostKey[] = "Host";
const char SshPortKey[] = "SshPort";
const char PortsSpecKey[] = "FreePortsSpec";
const char UserNameKey[] = "Uname";
const char AuthKey[] = "Authentication";
const char KeyFileKey[] = "KeyFile";
const char PasswordKey[] = "Password";
const char TimeoutKey[] = "Timeout";

//Mer specific stuff
const char MerVirtualMachine[] = "MerVirtualMachine";
const char MerMac[] = "MerMac";
const char MerSubnet[] = "MerSubnet";
const char MerSharedConfig[] = "MerSharedConfig";
const char MerSharedSsh[] = "MerSharedSsh";

AddDeviceOperation::AddDeviceOperation()
    : m_origin(0)
    , m_sdkProvided(false)
    , m_port(0)
    , m_autheticationType(0)
    , m_timeout(0)
    , m_machineType(0)
    , m_version(0)
{
}

QString AddDeviceOperation::name() const
{
    return QLatin1String("addDevice");
}

QString AddDeviceOperation::helpText() const
{
    return QLatin1String("add device to Qt Creator configuration");
}

QString AddDeviceOperation::param(const QString &text)
{
    return QLatin1String("--") + text.toLower();
}

QString AddDeviceOperation::argumentsHelpText() const
{
    const QString indent = QLatin1String("    ");
    return indent + param(QLatin1String(IdKey))+ QLatin1String(" <NAME>                            internal device ID (required).\n")
         + indent + param(QLatin1String(DisplayNameKey))+ QLatin1String(" <NAME>                                  device name (required).\n")
         + indent + param(QLatin1String(TypeKey)) + QLatin1String(" <NAME>                                device type (required).\n")
         + indent + indent + QLatin1String(Mer::Constants::MER_DEVICE_TYPE_I486) + QLatin1String("    for mer i486 target\n")
         + indent + indent + QLatin1String(Mer::Constants::MER_DEVICE_TYPE_ARM) + QLatin1String("     for mer arm target\n")
         + indent + param(QLatin1String(OriginKey)) + QLatin1String(" <manuallyAdded/autoDetected>          origin.\n")
         + indent + param(QLatin1String(SdkProvidedKey)) + QLatin1String(" <true/false>                sdk installer provided.\n")
         + indent + param(QLatin1String(MachineTypeKey)) + QLatin1String(" <hardware/emulator>                     machine type.\n")
         + indent + param(QLatin1String(HostKey)) + QLatin1String(" <NAME>                                  host name for ssh connection.\n")
         + indent + param(QLatin1String(SshPortKey)) + QLatin1String(" <NUMBER>                             port for ssh connection.\n")
         + indent + param(QLatin1String(UserNameKey)) + QLatin1String(" <NAME>                                 user name for ssh connection.\n")
         + indent + param(QLatin1String(AuthKey)) + QLatin1String(" <password/privateKey>         authentication Id type (required).\n")
         + indent + param(QLatin1String(KeyFileKey)) + QLatin1String(" <FILE>                               private key file (required).\n")
         + indent + param(QLatin1String(PasswordKey)) + QLatin1String(" <NAME>                              password for ssh connection.\n")
         + indent + param(QLatin1String(TimeoutKey)) + QLatin1String(" <NUMBER>                             timeout for ssh connection.\n")
         + indent + param(QLatin1String(PortsSpecKey)) + QLatin1String(" <NUMBER,NUMBER|NUMBER-NUMBER>  free ports.\n")
         + indent + param(QLatin1String(VersionKey)) + QLatin1String(" <NUMBER>                             version of the device.\n")
         + indent + param(QLatin1String(MerVirtualMachine)) + QLatin1String(" <STRING>                             name of virtual machine for device.\n")
         + indent + param(QLatin1String(MerMac)) + QLatin1String(" <STRING>                             mac of the device.\n")
         + indent + param(QLatin1String(MerSubnet)) + QLatin1String(" <STRING>                             subnet of the device.\n")
         + indent + param(QLatin1String(MerSharedSsh)) + QLatin1String(" <STRING>                             path of vm shared ssh.\n")
         + indent + param(QLatin1String(MerSharedConfig)) + QLatin1String(" <STRING>                             path of vm shared config.\n");
}

bool AddDeviceOperation::setArguments(const QStringList &args)
{
    for (int i = 0; i < args.count(); ++i) {
        const QString current = args.at(i);
        const QString next = ((i + 1) < args.count()) ? args.at(i + 1) : QString();
        if (current == param(QLatin1String(IdKey))) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_internalId = next.toLatin1();
            continue;
        }

        if (current == param(QLatin1String(DisplayNameKey))) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_displayName = next;
            continue;
        }

        if (current == param(QLatin1String(TypeKey))) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_type = next;
            continue;
        }

        if (current == param(QLatin1String(MachineTypeKey))) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_machineType = next == QLatin1String("emulator");
            continue;
        }

        if (current == param(QLatin1String(OriginKey))) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_origin = next == QLatin1String("manuallyAdded");
            continue;
        }

        if (current == param(QLatin1String(SdkProvidedKey))) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_sdkProvided = next == QLatin1String("true");
            continue;
        }

        if (current == param(QLatin1String(SshPortKey))) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_port = next.toInt();
            continue;
        }

        if (current == param(QLatin1String(HostKey))) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_host = next;
            continue;
        }

        if (current == param(QLatin1String(UserNameKey))) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_userName = next;
            continue;
        }

        if (current == param(QLatin1String(AuthKey))) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_autheticationType = next == QLatin1String("privateKey");
            continue;
        }

        if (current == param(QLatin1String(KeyFileKey))) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_privateKeyFile = next;
            continue;
        }

        if (current == param(QLatin1String(PasswordKey))) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_password = next;
            continue;
        }

        if (current == param(QLatin1String(TimeoutKey))) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_timeout = next.toInt();
            continue;
        }

        if (current == param(QLatin1String(PortsSpecKey))) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_freePorts = next;
            continue;
        }

        if (current == param(QLatin1String(VersionKey))) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_version = next.toInt();
            continue;
        }

        if (current == param(QLatin1String(MerVirtualMachine))) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_merVirtualMachine = next;
            continue;
        }

        if (current == param(QLatin1String(MerMac))) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_merMac = next;
            continue;
        }

        if (current == param(QLatin1String(MerSubnet))) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_merSubnet = next;
            continue;
        }
        if (current == param(QLatin1String(MerSharedSsh))) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_merSharedSshPath = next;
            continue;
        }
        if (current == param(QLatin1String(MerSharedConfig))) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_merSharedConfigPath = next;
            continue;
        }
    }

    const char MISSING[] = " parameter missing.";
    bool error = false;
    if (m_internalId.isEmpty()) {
        std::cerr << IdKey << MISSING << std::endl << std::endl;
        error = true;
    }
    if (m_displayName.isEmpty()) {
        std::cerr << DisplayNameKey << MISSING << std::endl << std::endl;
        error = true;
    }
    if (m_type.isEmpty()) {
        std::cerr << TypeKey << MISSING << std::endl << std::endl;
        error = true;
    }

    return !error;
}

int AddDeviceOperation::execute() const
{
    const QString devicesKey(QLatin1String("devices"));
    QVariantMap map = load(devicesKey);
    if (map.isEmpty())
        map = initializeDevices();

    if (map.isEmpty())
        std::cerr << "Error: Count found devices, file seems wrong." << std::endl;

    const QVariantMap result = addDevice(map,
                                         m_internalId,
                                         m_displayName,
                                         m_type,
                                         m_origin,
                                         m_sdkProvided,
                                         m_machineType,
                                         m_host,
                                         m_port,
                                         m_userName,
                                         m_autheticationType,
                                         m_password,
                                         m_privateKeyFile,
                                         m_timeout,
                                         m_freePorts,
                                         m_version,
                                         m_merVirtualMachine,
                                         m_merMac,
                                         m_merSubnet,
                                         m_merSharedSshPath,
                                         m_merSharedConfigPath);

    if (result.isEmpty() || map == result)
        return 2;

    return save(result, devicesKey) ? 0 : 3;
}

QVariantMap AddDeviceOperation::initializeDevices()
{
    QVariantMap map;
    QVariantList deviceList;
    map.insert(QLatin1String(DeviceListKey), deviceList);
    QVariantMap data;
    data.insert(QLatin1String(DeviceManagerKey), map);
    return data;
}

QVariantMap AddDeviceOperation::addDevice(const QVariantMap &map,
                                          const QByteArray &internalId,
                                          const QString &displayName,
                                          const QString &type,
                                          int origin,
                                          bool sdkProvided,
                                          int machineType,
                                          const QString &host,
                                          int port,
                                          const QString &userName,
                                          int authenticationType,
                                          const QString &password,
                                          const QString &privateKeyFile,
                                          int timeout,
                                          const QString &freePorts,
                                          int version,
                                          const QString &virtualMachine,
                                          const QString &merMac,
                                          const QString &merSubnet,
                                          const QString &sharedSshPath,
                                          const QString &sharedConfigPath)
{
    QVariantMap result = map;

    QVariantMap mapdevice = map.value(QLatin1String(DeviceManagerKey)).toMap();
    QVariantList deviceList = mapdevice.value(QLatin1String(DeviceListKey)).toList();

    foreach(const QVariant& m, deviceList){
        QStringList valueKeys = FindValueOperation::findValues(m.toMap(), QVariant(internalId));
        bool hasTarget = false;
        foreach (const QString &t, valueKeys) {
            if (t.endsWith(QLatin1String(IdKey))) {
                hasTarget = true;
                break;
            }
        }
        if (hasTarget) {
            std::cerr << "Error: InternalId " << qPrintable(QLatin1String(internalId)) << " already defined as device id." << std::endl;
            return result;
        }

        valueKeys = FindValueOperation::findValues(m.toMap(), QVariant(displayName));
        hasTarget = false;
        foreach (const QString &t, valueKeys) {
            if (t.endsWith(QLatin1String(DisplayNameKey))) {
                hasTarget = true;
                break;
            }
        }
        if (hasTarget) {
            std::cerr << "Error: Name " << qPrintable(displayName) << " already defined as device name." << std::endl;
            return result;
        }

    }

    QVariantMap data;
    data.insert(QLatin1String(IdKey), QVariant(internalId));
    data.insert(QLatin1String(DisplayNameKey), QVariant(displayName));
    data.insert(QLatin1String(TypeKey), QVariant(type));
    data.insert(QLatin1String(OriginKey),QVariant(origin));
    data.insert(QLatin1String(SdkProvidedKey),QVariant(sdkProvided));
    data.insert(QLatin1String(MachineTypeKey), QVariant(machineType));
    data.insert(QLatin1String(HostKey), QVariant(host));
    data.insert(QLatin1String(SshPortKey), QVariant(port));
    data.insert(QLatin1String(UserNameKey), QVariant(userName));
    data.insert(QLatin1String(AuthKey), QVariant(authenticationType));
    data.insert(QLatin1String(PasswordKey), QVariant(password));
    data.insert(QLatin1String(KeyFileKey), QVariant(privateKeyFile));
    data.insert(QLatin1String(TimeoutKey), QVariant(timeout));
    data.insert(QLatin1String(PortsSpecKey), QVariant(freePorts));
    data.insert(QLatin1String(VersionKey), QVariant(version));
    if(!virtualMachine.isEmpty())
        data.insert(QLatin1String(Mer::Constants::MER_DEVICE_VIRTUAL_MACHINE) , QVariant(virtualMachine));
    if(!merMac.isEmpty())
        data.insert(QLatin1String(Mer::Constants::MER_DEVICE_MAC), QVariant(merMac));
    if(!merSubnet.isEmpty())
        data.insert(QLatin1String(Mer::Constants::MER_DEVICE_SUBNET), QVariant(merSubnet));
    if(!sharedSshPath.isEmpty())
        data.insert(QLatin1String(Mer::Constants::MER_DEVICE_SHARED_SSH), QVariant(sharedSshPath));
    if(!sharedConfigPath.isEmpty())
        data.insert(QLatin1String(Mer::Constants::MER_DEVICE_SHARED_CONFIG), QVariant(sharedConfigPath));
    deviceList.append(data);

    QString key = QLatin1String(DeviceManagerKey) + QLatin1Char('/') + QLatin1String(DeviceListKey);
    result = RmKeysOperation::rmKeys(result,  QStringList() << key);
    return AddKeysOperation::addKeys(result, KeyValuePairList() << KeyValuePair(key, deviceList));
}
