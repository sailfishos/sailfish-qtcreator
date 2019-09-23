/****************************************************************************
 **
 ** Copyright (C) 2016 Jolla Ltd.
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

#include "rmsfdkdevicemodeloperation.h"
#include "addsfdkdevicemodeloperation.h"
#include "addsfdkemulatoroperation.h"
#include "getoperation.h"
#include "rmkeysoperation.h"
#include "addkeysoperation.h"
#include "sfdkutils.h"
#include "../../libs/sfdk/sfdkconstants.h"
#include <iostream>

namespace C = Sfdk::Constants;

RmSfdkDeviceModelOperation::RmSfdkDeviceModelOperation()
{
}

QString RmSfdkDeviceModelOperation::name() const
{
    return QLatin1String("rmSfdkDeviceModel");
}

QString RmSfdkDeviceModelOperation::helpText() const
{
    return QLatin1String("remove an Sfdk emulator device model");
}

QString RmSfdkDeviceModelOperation::argumentsHelpText() const
{
    return QLatin1String("    --name <NAME>              name of the device model to remove (required).\n");
}

bool RmSfdkDeviceModelOperation::setArguments(const QStringList &args)
{
    if (args.count() != 2)
        return false;
    if (args.at(0) != QLatin1String("--name"))
        return false;

    m_name = args.at(1);

    if (m_name.isEmpty())
        std::cerr << "No --name given." << std::endl << std::endl;

    return !m_name.isEmpty();
}

int RmSfdkDeviceModelOperation::execute() const
{
    useSfdkSettingsPath();

    QVariantMap map = load(QLatin1String("SfdkEmulators"));
    if (map.isEmpty())
        map = AddSfdkDeviceModelOperation::initializeDeviceModels();

    QVariantMap result = removeDeviceModel(map, m_name);

    if (result == map)
        return 2;

    return save(result, QLatin1String("SfdkEmulators")) ? 0 : 3;
}

QVariantMap RmSfdkDeviceModelOperation::removeDeviceModel(const QVariantMap &map, const QString &name)
{
    QVariantList deviceModelList;
    bool ok;
    int version = GetOperation::get(map, QLatin1String(C::EMULATORS_VERSION_KEY)).toInt(&ok);
    if (!ok) {
        std::cerr << "Error: The version found in map is not an integer." << std::endl;
        return map;
    }
    QString installDir = GetOperation::get(map, QLatin1String(C::EMULATORS_INSTALL_DIR_KEY)).toString();
    if (installDir.isEmpty()) {
        std::cerr << "Error: No install-directory found in map or empty." << std::endl;
        return map;
    }
    int count = GetOperation::get(map, QLatin1String(C::DEVICE_MODELS_COUNT_KEY)).toInt(&ok);
    if (!ok) {
        std::cerr << "Error: The count found in map is not an integer." << std::endl;
        return map;
    }

    bool removed = false;
    for (int i = 0; i < count; ++i) {
        const QString deviceModel = QString::fromLatin1(C::DEVICE_MODELS_DATA_KEY_PREFIX) + QString::number(i);
        QVariantMap deviceModelMap = map.value(deviceModel).toMap();
        if (deviceModelMap.value(QLatin1String(C::DEVICE_MODEL_NAME)).toString() == name) {
            removed = true;
            continue;
        }
        deviceModelList << deviceModelMap;
    }
    if (!removed) {
        std::cerr << "Error: Device model was not found." << std::endl;
        return map;
    }

    // insert data:
    KeyValuePairList data;
    data << KeyValuePair(QStringList() << QLatin1String(C::EMULATORS_VERSION_KEY), QVariant(version));
    data << KeyValuePair(QStringList() << QLatin1String(C::EMULATORS_INSTALL_DIR_KEY), QVariant(installDir));
    data << KeyValuePair(QStringList() << QLatin1String(C::DEVICE_MODELS_COUNT_KEY), QVariant(count - 1));

    for (int i = 0; i < deviceModelList.count(); ++i) {
        data << KeyValuePair(QStringList()
                << QString::fromLatin1(C::DEVICE_MODELS_DATA_KEY_PREFIX) + QString::number(i),
                deviceModelList.at(i));
    }

    // copy emulators
    int emulatorsCount = GetOperation::get(map, QLatin1String(C::EMULATORS_COUNT_KEY)).toInt(&ok);
    if (!ok) {
        std::cerr << "Error: The emulators count found in map is not an integer." << std::endl;
        return map;
    };
    data << KeyValuePair(QLatin1String(C::EMULATORS_COUNT_KEY), QVariant(emulatorsCount));
    for (int i = 0; i < emulatorsCount; ++i) {
        const QString emulatorKey = QString::fromLatin1(C::EMULATORS_DATA_KEY_PREFIX) + QString::number(i);
        data << KeyValuePair(emulatorKey, map.value(emulatorKey));
    }

    QVariantMap result;
    return AddKeysOperation::addKeys(result, data);
}

#ifdef WITH_TESTS
bool RmSfdkDeviceModelOperation::test() const
{
    QVariantMap map = AddSfdkDeviceModelOperation::addDeviceModel(AddSfdkDeviceModelOperation::initializeDeviceModels(),
                                                                 QLatin1String("Test Device 1"), 500, 1000, 50, 100,
                                                                 QLatin1String("Test dconf content 1"));

    map = AddSfdkDeviceModelOperation::addDeviceModel(map, QLatin1String("Test Device 2"), 250, 500, 25, 50,
                                                     QLatin1String("Test dconf content 2"));

    map = AddSfdkEmulatorOperation::addEmulator(map,
                 QUrl("sfdkvm:VirtualBox#testEmulator"),
                 QLatin1String("testSnapshot"),
                 true,
                 QLatin1String("/test/sharedSshPath"),
                 QLatin1String("/test/sharedConfigPath"),
                 QLatin1String("host"),
                 QLatin1String("user"),
                 QLatin1String("/test/privateKey"),
                 22,
                 QLatin1String("10234"),
                 QLatin1String("10000-10010"),
                 QLatin1String("de:ad:be:ef:fe:ed"),
                 QLatin1String("10.220.220"),
                 QVariantMap(),
                 false);

    const QString deviceModel1 = QString::fromLatin1(C::DEVICE_MODELS_DATA_KEY_PREFIX) + QString::number(0);
    const QString deviceModel2 = QString::fromLatin1(C::DEVICE_MODELS_DATA_KEY_PREFIX) + QString::number(1);
    const QString emulator1 = QString::fromLatin1(C::EMULATORS_DATA_KEY_PREFIX) + QString::number(0);

    if (map.count() != 7
            || !map.contains(QLatin1String(C::EMULATORS_VERSION_KEY))
            || map.value(QLatin1String(C::EMULATORS_VERSION_KEY)).toInt() != -1
            || !map.contains(QLatin1String(C::EMULATORS_INSTALL_DIR_KEY))
            || map.value(QLatin1String(C::EMULATORS_INSTALL_DIR_KEY)).toString()
                != QLatin1String("/dev/null")
            || !map.contains(QLatin1String(C::EMULATORS_COUNT_KEY))
            || map.value(QLatin1String(C::EMULATORS_COUNT_KEY)).toInt() != 1
            || !map.contains(QLatin1String(C::DEVICE_MODELS_COUNT_KEY))
            || map.value(QLatin1String(C::DEVICE_MODELS_COUNT_KEY)).toInt() != 2
            || !map.contains(deviceModel1)
            || !map.contains(deviceModel2)
            || !map.contains(emulator1))
        return false;

    QVariantMap result = removeDeviceModel(map, QLatin1String("Test Device 2"));
    if (result.count() != 6
            || result.contains(deviceModel2)
            || !result.contains(deviceModel1)
            || !result.contains(emulator1)
            || !result.contains(QLatin1String(C::DEVICE_MODELS_COUNT_KEY))
            || result.value(QLatin1String(C::DEVICE_MODELS_COUNT_KEY)).toInt() != 1
            || !result.contains(QLatin1String(C::EMULATORS_COUNT_KEY))
            || result.value(QLatin1String(C::EMULATORS_COUNT_KEY)).toInt() != 1
            || !result.contains(QLatin1String(C::EMULATORS_VERSION_KEY))
            || result.value(QLatin1String(C::EMULATORS_VERSION_KEY)).toInt() != -1
            || !map.contains(QLatin1String(C::EMULATORS_INSTALL_DIR_KEY))
            || map.value(QLatin1String(C::EMULATORS_INSTALL_DIR_KEY)).toString()
                != QLatin1String("/dev/null"))
        return false;

    QString deviceModel = QString::fromLatin1(C::DEVICE_MODELS_DATA_KEY_PREFIX) + QString::number(0);
    QVariantMap deviceModelMap = result.value(deviceModel).toMap();
    if (deviceModelMap.value(QLatin1String(C::DEVICE_MODEL_NAME)).toString() == QLatin1String("Test Device 2"))
        return false;

    result = removeDeviceModel(map, QLatin1String("unknown"));
    if (result != map)
        return false;

    result = removeDeviceModel(map, QLatin1String("Test Device 1"));
    if (result.count() != 6
              || result.contains(deviceModel2)
              || !result.contains(deviceModel1)
              || !result.contains(emulator1)
              || !result.contains(QLatin1String(C::DEVICE_MODELS_COUNT_KEY))
              || result.value(QLatin1String(C::DEVICE_MODELS_COUNT_KEY)).toInt() != 1
              || !result.contains(QLatin1String(C::EMULATORS_COUNT_KEY))
              || result.value(QLatin1String(C::EMULATORS_COUNT_KEY)).toInt() != 1
              || !result.contains(QLatin1String(C::EMULATORS_VERSION_KEY))
              || result.value(QLatin1String(C::EMULATORS_VERSION_KEY)).toInt() != -1
              || !map.contains(QLatin1String(C::EMULATORS_INSTALL_DIR_KEY))
              || map.value(QLatin1String(C::EMULATORS_INSTALL_DIR_KEY)).toString()
                  != QLatin1String("/dev/null"))
        return false;

    deviceModel = QString::fromLatin1(C::DEVICE_MODELS_DATA_KEY_PREFIX) + QString::number(0);
    deviceModelMap = result.value(deviceModel).toMap();
    if (deviceModelMap.value(QLatin1String(C::DEVICE_MODEL_NAME)).toString() == QLatin1String("Test Device 1"))
        return false;

    return true;
}
#endif
