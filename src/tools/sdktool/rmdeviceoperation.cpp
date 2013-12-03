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

#include "rmdeviceoperation.h"
#include "adddeviceoperation.h"
#include "addkeysoperation.h"
#include "rmkeysoperation.h"
#include "getoperation.h"
#include <QDebug>
#include <iostream>

const char IdKey[] = "InternalId";
const char DeviceManagerKey[] = "DeviceManager";
const char DeviceListKey[] = "DeviceList";

RmDeviceOperation::RmDeviceOperation()
{
}

QString RmDeviceOperation::name() const
{
    return QLatin1String("removeDevice");
}

QString RmDeviceOperation::helpText() const
{
    return QLatin1String("remove a Kit to Qt Creator");
}

QString RmDeviceOperation::argumentsHelpText() const
{
    return QLatin1String("    --id <ID>                                  id of the device (required).\n");
}

bool RmDeviceOperation::setArguments(const QStringList &args)
{
    if (args.count() != 2)
        return false;
    if (args.at(0) != QLatin1String("--id"))
        return false;

    m_deviceId = args.at(1);

    if (m_deviceId.isEmpty())
        std::cerr << "No device id given." << std::endl << std::endl;

    return !m_deviceId.isEmpty();
}

int RmDeviceOperation::execute() const
{
    QVariantMap map = load(QLatin1String("devices"));
    if (map.isEmpty())
        map = AddDeviceOperation::initializeDevices();

    QVariantMap result = removeDevice(map, m_deviceId);

    if (result == map)
        return 2;

    return save(result,QLatin1String("devices")) ? 0 : 3;
}

QVariantMap RmDeviceOperation::removeDevice(const QVariantMap &map, const QString &id)
{
    QVariantMap result = map;
    QVariantMap managerMap = GetOperation::get(result, QLatin1String(DeviceManagerKey)).toMap();

    QVariantList deviceList;
    QVariantList devices = managerMap.value(QLatin1String(DeviceListKey)).toList();

    foreach (const QVariant &deviceMap, devices) {
        QString deviceId = GetOperation::get(deviceMap.toMap(), QLatin1String(IdKey)).toString();
        if ( deviceId == id)
            continue;
        deviceList.append(deviceMap);
    }

    if (deviceList.count() == devices.count()) {
        std::cerr << "Error: Device was not found." << std::endl;
        return map;
    }
    QString key = QLatin1String(DeviceManagerKey) + QLatin1Char('/') + QLatin1String(DeviceListKey);
    result = RmKeysOperation::rmKeys(result,  QStringList() << key);
    return AddKeysOperation::addKeys(result,  KeyValuePairList() << KeyValuePair(key,deviceList));
}


#ifdef WITH_TESTS
bool RmDeviceOperation::test() const
{
    QVariantMap map = AddDeviceOperation::addDevice(AddDeviceOperation::initializeDevices(),
                                                 "testEmulator1",
                                                 QLatin1String("testEmulator1"),
                                                 QLatin1String("test.Type"),
                                                 0,
                                                 false,
                                                 0,
                                                 QLatin1String("localhost"),
                                                 22,
                                                 QLatin1String("user"),
                                                 0,
                                                 QLatin1String("/test/password"),
                                                 QLatin1String("/test/privateKey"),
                                                 0,
                                                 QLatin1String("none"),
                                                 1,
                                                 QLatin1String("emualtorVM"),
                                                 QLatin1String("/test/merMac"),
                                                 QLatin1String("/test/subnet"),
                                                 QLatin1String("/test/sharedSshPath"),
                                                 QLatin1String("/test/sharedConfigPath"));

    map = AddDeviceOperation::addDevice(map,
                                        "testEmulator2",
                                        QLatin1String("testEmulator2"),
                                        QLatin1String("test.Type"),
                                        0,
                                        false,
                                        0,
                                        QLatin1String("localhost"),
                                        22,
                                        QLatin1String("user"),
                                        0,
                                        QLatin1String("/test/password"),
                                        QLatin1String("/test/privateKey"),
                                        0,
                                        QLatin1String("none"),
                                        1,
                                        QLatin1String("emualtorVM"),
                                        QLatin1String("/test/merMac"),
                                        QLatin1String("/test/subnet"),
                                        QLatin1String("/test/sharedSshPath"),
                                        QLatin1String("/test/sharedConfigPath"));


    if (map.count() != 1
            || !map.contains(QLatin1String(DeviceManagerKey)))
        return false;

    QVariantMap managerMap = GetOperation::get(map, QLatin1String(DeviceManagerKey)).toMap();
    QVariantList devices = managerMap.value(QLatin1String(DeviceListKey)).toList();
    if (devices.count() != 2 )
        return false;

    foreach (const QVariant &deviceMap, devices) {
        QString deviceId = GetOperation::get(deviceMap.toMap(), QLatin1String(IdKey)).toString();
        if ( deviceId != QLatin1String("testEmulator2") && deviceId != QLatin1String("testEmulator1"))
            return false;
    }

    QVariantMap result = removeDevice(map, QLatin1String("testEmulator2"));
    if (result.count() != 1
            || !result.contains(QLatin1String(DeviceManagerKey)))
        return false;

    managerMap = GetOperation::get(result, QLatin1String(DeviceManagerKey)).toMap();
    devices = managerMap.value(QLatin1String(DeviceListKey)).toList();
    if (devices.count() != 1 )
        return false;

    foreach (const QVariant &deviceMap, devices) {
        QString deviceId = GetOperation::get(deviceMap.toMap(), QLatin1String(IdKey)).toString();
        if ( deviceId != QLatin1String("testEmulator1"))
            return false;
    }

    result = removeDevice(map, QLatin1String("unknown"));
    if (result != map)
        return false;

    result = removeDevice(map, QLatin1String("testEmulator1"));
    if (map.count() != 1
            || !map.contains(QLatin1String(DeviceManagerKey)))
        return false;

    managerMap = GetOperation::get(result, QLatin1String(DeviceManagerKey)).toMap();
    devices = managerMap.value(QLatin1String(DeviceListKey)).toList();
    if (devices.count() != 1 )
        return false;

    foreach (const QVariant &deviceMap, devices) {
        QString deviceId = GetOperation::get(deviceMap.toMap(), QLatin1String(IdKey)).toString();
        if ( deviceId != QLatin1String("testEmulator2"))
            return false;
    }

    return true;
}
#endif


