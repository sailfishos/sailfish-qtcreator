/****************************************************************************
 **
 ** Copyright (C) 2016 Jolla Ltd.
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

#include "rmmerdevicemodeloperation.h"
#include "addmerdevicemodeloperation.h"
#include "getoperation.h"
#include "rmkeysoperation.h"
#include "addkeysoperation.h"
#include "../../plugins/mer/merconstants.h"
#include <iostream>

RmMerDeviceModelOperation::RmMerDeviceModelOperation()
{
}

QString RmMerDeviceModelOperation::name() const
{
    return QLatin1String("removeMerDeviceModel");
}

QString RmMerDeviceModelOperation::helpText() const
{
    return QLatin1String("remove a Mer emulator device model from Qt Creator");
}

QString RmMerDeviceModelOperation::argumentsHelpText() const
{
    return QLatin1String("    --name <NAME>              name of the device model to remove (required).\n");
}

bool RmMerDeviceModelOperation::setArguments(const QStringList &args)
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

int RmMerDeviceModelOperation::execute() const
{
    QVariantMap map = load(QLatin1String("mersdk-device-models"));
    if (map.isEmpty())
        map = AddMerDeviceModelOperation::initializeDeviceModels();

    QVariantMap result = removeDeviceModel(map, m_name);

    if (result == map)
        return 2;

    return save(result, QLatin1String("mersdk-device-models")) ? 0 : 3;
}

QVariantMap RmMerDeviceModelOperation::removeDeviceModel(const QVariantMap &map, const QString &name)
{
    QVariantMap result = AddMerDeviceModelOperation::initializeDeviceModels();

    QVariantList deviceModelList;
    bool ok;
    int count = GetOperation::get(map, QLatin1String(Mer::Constants::MER_DEVICE_MODELS_COUNT_KEY)).toInt(&ok);
    if (!ok) {
        std::cerr << "Error: The count found in map is not an integer." << std::endl;
        return map;
    }

    for (int i = 0; i < count; ++i) {
        const QString deviceModel = QString::fromLatin1(Mer::Constants::MER_DEVICE_MODELS_DATA_KEY) + QString::number(i);
        QVariantMap deviceModelMap = map.value(deviceModel).toMap();
        if (deviceModelMap.value(QLatin1String(Mer::Constants::MER_DEVICE_MODEL_NAME)).toString() == name) {
            continue;
        }
        deviceModelList << deviceModelMap;
    }
    if (deviceModelList.count() == map.count() - 2) {
        std::cerr << "Error: device model was not found." << std::endl;
        return map;
    }

    // remove data:
    QStringList toRemove;
    toRemove << QLatin1String(Mer::Constants::MER_DEVICE_MODELS_COUNT_KEY);
    result = RmKeysOperation::rmKeys(result, toRemove);

    // insert data:
    KeyValuePairList data;
    data << KeyValuePair(QStringList() << QLatin1String(Mer::Constants::MER_DEVICE_MODELS_COUNT_KEY), QVariant(count - 1));

    for (int i = 0; i < deviceModelList.count(); ++i)
        data << KeyValuePair(QStringList() << QString::fromLatin1(Mer::Constants::MER_DEVICE_MODELS_DATA_KEY) + QString::number(i),
                             deviceModelList.at(i));

    return AddKeysOperation::addKeys(result, data);
}

#ifdef WITH_TESTS
bool RmMerDeviceModelOperation::test() const
{
    QVariantMap map = AddMerDeviceModelOperation::addDeviceModel(AddMerDeviceModelOperation::initializeDeviceModels(),
                                                                 QLatin1String("Test Device 1"), 500, 1000, 50, 100);

    map = AddMerDeviceModelOperation::addDeviceModel(map, QLatin1String("Test Device 2"), 250, 500, 25, 50);


    const QString deviceModel1 = QString::fromLatin1(Mer::Constants::MER_DEVICE_MODELS_DATA_KEY) + QString::number(0);
    const QString deviceModel2 = QString::fromLatin1(Mer::Constants::MER_DEVICE_MODELS_DATA_KEY) + QString::number(1);


    if (map.count() != 4
            || !map.contains(QLatin1String(Mer::Constants::MER_DEVICE_MODELS_FILE_VERSION_KEY))
            || map.value(QLatin1String(Mer::Constants::MER_DEVICE_MODELS_FILE_VERSION_KEY)).toInt() != 1
            || !map.contains(QLatin1String(Mer::Constants::MER_DEVICE_MODELS_COUNT_KEY))
            || map.value(QLatin1String(Mer::Constants::MER_DEVICE_MODELS_COUNT_KEY)).toInt() != 2
            || !map.contains(deviceModel1)
            || !map.contains(deviceModel2))
        return false;

    QVariantMap result = removeDeviceModel(map, QLatin1String("Test Device 2"));
    if (result.count() != 3
            || result.contains(deviceModel2)
            || !result.contains(deviceModel1)
            || !result.contains(QLatin1String(Mer::Constants::MER_DEVICE_MODELS_COUNT_KEY))
            || result.value(QLatin1String(Mer::Constants::MER_DEVICE_MODELS_COUNT_KEY)).toInt() != 1
            || !result.contains(QLatin1String(Mer::Constants::MER_DEVICE_MODELS_FILE_VERSION_KEY))
            || result.value(QLatin1String(Mer::Constants::MER_DEVICE_MODELS_FILE_VERSION_KEY)).toInt() != 1)
        return false;

    QString deviceModel = QString::fromLatin1(Mer::Constants::MER_DEVICE_MODELS_DATA_KEY) + QString::number(0);
    QVariantMap deviceModelMap = result.value(deviceModel).toMap();
    if (deviceModelMap.value(QLatin1String(Mer::Constants::MER_DEVICE_MODEL_NAME)).toString() == QLatin1String("Test Device 2"))
        return false;

    result = removeDeviceModel(map, QLatin1String("unknown"));
    if (result != map)
        return false;

    result = removeDeviceModel(map, QLatin1String("Test Device 1"));
    if (result.count() != 3
              || result.contains(deviceModel2)
              || !result.contains(deviceModel1)
              || !result.contains(QLatin1String(Mer::Constants::MER_DEVICE_MODELS_COUNT_KEY))
              || result.value(QLatin1String(Mer::Constants::MER_DEVICE_MODELS_COUNT_KEY)).toInt() != 1
              || !result.contains(QLatin1String(Mer::Constants::MER_DEVICE_MODELS_FILE_VERSION_KEY))
              || result.value(QLatin1String(Mer::Constants::MER_DEVICE_MODELS_FILE_VERSION_KEY)).toInt() != 1)
        return false;

    deviceModel = QString::fromLatin1(Mer::Constants::MER_DEVICE_MODELS_DATA_KEY) + QString::number(0);
    deviceModelMap = result.value(deviceModel).toMap();
    if (deviceModelMap.value(QLatin1String(Mer::Constants::MER_DEVICE_MODEL_NAME)).toString() == QLatin1String("Test Device 1"))
        return false;


    return true;
}
#endif
