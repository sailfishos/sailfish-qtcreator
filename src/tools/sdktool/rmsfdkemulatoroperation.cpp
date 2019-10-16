/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Copyright (C) 2013-2014,2017,2019 Jolla Ltd.
** Copyright (C) 2019 Open Mobile Platform LLC.
** Contact: http://jolla.com/
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

#include "rmsfdkemulatoroperation.h"
#include "addsfdkdevicemodeloperation.h"
#include "addsfdkemulatoroperation.h"
#include "getoperation.h"
#include "rmkeysoperation.h"
#include "addkeysoperation.h"
#include "sfdkutils.h"

#include "../../libs/sfdk/sfdkconstants.h"

#include <QDateTime>

#include <iostream>

namespace C = Sfdk::Constants;

RmSfdkEmulatorOperation::RmSfdkEmulatorOperation()
{
}

QString RmSfdkEmulatorOperation::name() const
{
    return QLatin1String("rmSfdkEmulator");
}

QString RmSfdkEmulatorOperation::helpText() const
{
    return QLatin1String("remove an Sfdk emulator");
}

QString RmSfdkEmulatorOperation::argumentsHelpText() const
{
    return QLatin1String("    --vm-uri <URI>                emulator virtual machine URI (required).\n");
}

bool RmSfdkEmulatorOperation::setArguments(const QStringList &args)
{
    if (args.count() != 2)
        return false;
    if (args.at(0) != QLatin1String("--vm-uri"))
        return false;

    m_vmUri = QUrl(args.at(1));

    if (m_vmUri.isEmpty())
        std::cerr << "No URI given." << std::endl << std::endl;

    return !m_vmUri.isEmpty();
}

int RmSfdkEmulatorOperation::execute() const
{
    useSfdkSettingsPath();

    QVariantMap map = load(QLatin1String("SfdkEmulators"));
    if (map.isEmpty())
        return 2;

    QVariantMap result = removeEmulator(map, m_vmUri);

    if (result == map)
        return 2;

    return save(result, QLatin1String("SfdkEmulators")) ? 0 : 3;
}

QVariantMap RmSfdkEmulatorOperation::removeEmulator(const QVariantMap &map, const QUrl &vmUri)
{
    QVariantList emulatorList;
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
    int count = GetOperation::get(map, QLatin1String(C::EMULATORS_COUNT_KEY)).toInt(&ok);
    if (!ok) {
        std::cerr << "Error: The count found in map is not an integer." << std::endl;
        return map;
    }

    bool removed = false;
    for (int i = 0; i < count; ++i) {
        const QString emulator = QString::fromLatin1(C::EMULATORS_DATA_KEY_PREFIX) + QString::number(i);
        QVariantMap emulatorMap = map.value(emulator).toMap();
        if (emulatorMap.value(QLatin1String(C::EMULATOR_VM_URI)).toUrl() == vmUri) {
            removed = true;
            continue;
        }
        emulatorList << emulatorMap;
    }
    if (!removed) {
        std::cerr << "Error: Emulator was not found." << std::endl;
        return map;
    }

    // insert data:
    KeyValuePairList data;
    data << KeyValuePair(QStringList() << QLatin1String(C::EMULATORS_VERSION_KEY), QVariant(version));
    data << KeyValuePair(QStringList() << QLatin1String(C::EMULATORS_INSTALL_DIR_KEY), QVariant(installDir));
    data << KeyValuePair(QStringList() << QLatin1String(C::EMULATORS_COUNT_KEY), QVariant(count - 1));

    for (int i = 0; i < emulatorList.count(); ++i) {
        data << KeyValuePair(QStringList()
                << QString::fromLatin1(C::EMULATORS_DATA_KEY_PREFIX) + QString::number(i),
                emulatorList.at(i));
    }

    // copy device models
    int deviceModelsCount = GetOperation::get(map, QLatin1String(C::DEVICE_MODELS_COUNT_KEY)).toInt(&ok);
    if (!ok) {
        std::cerr << "Error: The device models count found in map is not an integer." << std::endl;
        return map;
    };
    data << KeyValuePair(QStringList() << QLatin1String(C::DEVICE_MODELS_COUNT_KEY), QVariant(deviceModelsCount));
    for (int i = 0; i < deviceModelsCount; ++i) {
        const QString deviceModelKey = QString::fromLatin1(C::DEVICE_MODELS_DATA_KEY_PREFIX) + QString::number(i);
        data << KeyValuePair(deviceModelKey, map.value(deviceModelKey));
    }

    QVariantMap result;
    return AddKeysOperation::addKeys(result, data);
}

#ifdef WITH_TESTS
bool RmSfdkEmulatorOperation::test() const
{
    const auto now = QDateTime::currentDateTime();

    QVariantMap map = AddSfdkEmulatorOperation::initializeEmulators(2, QLatin1String("/dir"));

    map = AddSfdkDeviceModelOperation::addDeviceModel(map,
                 QLatin1String("Test Device"), 500, 1000, 50, 100, QLatin1String("Test dconf"));

    map = AddSfdkEmulatorOperation::addEmulator(map,
                 QUrl("sfdkvm:VirtualBox#testEmulator"),
                 now,
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

    map = AddSfdkEmulatorOperation::addEmulator(map,
                 QUrl("sfdkvm:VirtualBox#testEmulator2"),
                 now,
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

    const QString emulator1 = QString::fromLatin1(C::EMULATORS_DATA_KEY_PREFIX) + QString::number(0);
    const QString emulator2 = QString::fromLatin1(C::EMULATORS_DATA_KEY_PREFIX) + QString::number(1);
    const QString deviceModel1 = QString::fromLatin1(C::DEVICE_MODELS_DATA_KEY_PREFIX) + QString::number(0);

    if (map.count() != 7
            || !map.contains(QLatin1String(C::EMULATORS_VERSION_KEY))
            || map.value(QLatin1String(C::EMULATORS_VERSION_KEY)).toInt() != 2
            || !map.contains(QLatin1String(C::EMULATORS_INSTALL_DIR_KEY))
            || map.value(QLatin1String(C::EMULATORS_INSTALL_DIR_KEY)).toString()
                != QLatin1String("/dir")
            || !map.contains(QLatin1String(C::EMULATORS_COUNT_KEY))
            || map.value(QLatin1String(C::EMULATORS_COUNT_KEY)).toInt() != 2
            || !map.contains(QLatin1String(C::DEVICE_MODELS_COUNT_KEY))
            || map.value(QLatin1String(C::DEVICE_MODELS_COUNT_KEY)).toInt() != 1
            || !map.contains(emulator1)
            || !map.contains(emulator2)
            || !map.contains(deviceModel1))
        return false;

    QVariantMap result = removeEmulator(map, QUrl("sfdkvm:VirtualBox#testEmulator2"));
    if (result.count() != 6
            || result.contains(emulator2)
            || !result.contains(emulator1)
            || !result.contains(deviceModel1)
            || !result.contains(QLatin1String(C::EMULATORS_COUNT_KEY))
            || result.value(QLatin1String(C::EMULATORS_COUNT_KEY)).toInt() != 1
            || !result.contains(QLatin1String(C::DEVICE_MODELS_COUNT_KEY))
            || result.value(QLatin1String(C::DEVICE_MODELS_COUNT_KEY)).toInt() != 1
            || !result.contains(QLatin1String(C::EMULATORS_VERSION_KEY))
            || result.value(QLatin1String(C::EMULATORS_VERSION_KEY)).toInt() != 2
            || !map.contains(QLatin1String(C::EMULATORS_INSTALL_DIR_KEY))
            || map.value(QLatin1String(C::EMULATORS_INSTALL_DIR_KEY)).toString()
                != QLatin1String("/dir"))
        return false;

    QString emulator = QString::fromLatin1(C::EMULATORS_DATA_KEY_PREFIX) + QString::number(0);
    QVariantMap emulatorMap = result.value(emulator).toMap();
    if (emulatorMap.value(QLatin1String(C::EMULATOR_VM_URI)).toUrl() == QUrl("sfdkvm:VirtualBox#testEmulator2"))
        return false;

    result = removeEmulator(map, QUrl("sfdkvm:VirtualBox#unknown"));
    if (result != map)
        return false;

    result = removeEmulator(map, QUrl("sfdkvm:VirtualBox#testEmulator"));
    if (result.count() != 6
              || result.contains(emulator2)
              || !result.contains(emulator1)
              || !result.contains(deviceModel1)
              || !result.contains(QLatin1String(C::EMULATORS_COUNT_KEY))
              || result.value(QLatin1String(C::EMULATORS_COUNT_KEY)).toInt() != 1
              || !result.contains(QLatin1String(C::DEVICE_MODELS_COUNT_KEY))
              || result.value(QLatin1String(C::DEVICE_MODELS_COUNT_KEY)).toInt() != 1
              || !result.contains(QLatin1String(C::EMULATORS_VERSION_KEY))
              || result.value(QLatin1String(C::EMULATORS_VERSION_KEY)).toInt() != 2
              || !map.contains(QLatin1String(C::EMULATORS_INSTALL_DIR_KEY))
              || map.value(QLatin1String(C::EMULATORS_INSTALL_DIR_KEY)).toString()
                  != QLatin1String("/dir"))
        return false;

    emulator = QString::fromLatin1(C::EMULATORS_DATA_KEY_PREFIX) + QString::number(0);
    emulatorMap = result.value(emulator).toMap();
    if (emulatorMap.value(QLatin1String(C::EMULATOR_VM_URI)).toUrl() == QUrl("sfdkvm:VirtualBox#testEmulator"))
        return false;

    QString deviceModel = QString::fromLatin1(C::DEVICE_MODELS_DATA_KEY_PREFIX) + QString::number(0);
    QVariantMap deviceModelMap = result.value(deviceModel).toMap();
    if (deviceModelMap.value(QLatin1String(C::DEVICE_MODEL_NAME)).toString() != QLatin1String("Test Device"))
        return false;

    return true;
}
#endif
