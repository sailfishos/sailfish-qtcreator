/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Copyright (C) 2013-2014,2017,2019 Jolla Ltd.
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

#include "rmmersdkoperation.h"
#include "addmersdkoperation.h"
#include "getoperation.h"
#include "rmkeysoperation.h"
#include "addkeysoperation.h"
#include "../../plugins/mer/merconstants.h"
#include <iostream>

RmMerSdkOperation::RmMerSdkOperation()
{
}

QString RmMerSdkOperation::name() const
{
    return QLatin1String("removeMerSdk");
}

QString RmMerSdkOperation::helpText() const
{
    return QLatin1String("remove a MerSdk from Qt Creator");
}

QString RmMerSdkOperation::argumentsHelpText() const
{
    return QLatin1String("    --vm-name <NAME>              mer sdk virtual machine name (required).\n");
}

bool RmMerSdkOperation::setArguments(const QStringList &args)
{
    if (args.count() != 2)
        return false;
    if (args.at(0) != QLatin1String("--vm-name"))
        return false;

    m_vmName = args.at(1);

    if (m_vmName.isEmpty())
        std::cerr << "No sdk given." << std::endl << std::endl;

    return !m_vmName.isEmpty();
}

int RmMerSdkOperation::execute() const
{
    QVariantMap map = load(QLatin1String("mersdk"));
    if (map.isEmpty())
        return 2;

    QVariantMap result = removeSdk(map, m_vmName);

    if (result == map)
        return 2;

    return save(result,QLatin1String("mersdk")) ? 0 : 3;
}

QVariantMap RmMerSdkOperation::removeSdk(const QVariantMap &map, const QString &sdkName)
{
    QVariantList sdkList;
    bool ok;
    int version = GetOperation::get(map, QLatin1String(Mer::Constants::MER_SDK_FILE_VERSION_KEY)).toInt(&ok);
    if (!ok) {
        std::cerr << "Error: The version found in map is not an integer." << std::endl;
        return map;
    }
    int count = GetOperation::get(map, QLatin1String(Mer::Constants::MER_SDK_COUNT_KEY)).toInt(&ok);
    if (!ok) {
        std::cerr << "Error: The count found in map is not an integer." << std::endl;
        return map;
    }

    for (int i = 0; i < count; ++i) {
        const QString sdk = QString::fromLatin1(Mer::Constants::MER_SDK_DATA_KEY) + QString::number(i);
        QVariantMap sdkMap = map.value(sdk).toMap();
        if (sdkMap.value(QLatin1String(Mer::Constants::VM_NAME)).toString() == sdkName) {
            continue;
        }
        sdkList << sdkMap;
    }
    if (sdkList.count() == map.count() - 2) {
        std::cerr << "Error: Sdk was not found." << std::endl;
        return map;
    }

    // insert data:
    KeyValuePairList data;
    data << KeyValuePair(QStringList() << QLatin1String(Mer::Constants::MER_SDK_FILE_VERSION_KEY), QVariant(version));
    data << KeyValuePair(QStringList() << QLatin1String(Mer::Constants::MER_SDK_COUNT_KEY), QVariant(count - 1));

    for (int i = 0; i < sdkList.count(); ++i)
        data << KeyValuePair(QStringList() << QString::fromLatin1(Mer::Constants::MER_SDK_DATA_KEY) + QString::number(i),
                             sdkList.at(i));

    QVariantMap result;
    return AddKeysOperation::addKeys(result, data);
}

#ifdef WITH_TESTS
bool RmMerSdkOperation::test() const
{
    QVariantMap map = AddMerSdkOperation::addSdk(AddMerSdkOperation::initializeSdks(2),
                                                 QLatin1String("testSdk"), true,
                                                 QLatin1String("/test/sharedHomePath"),
                                                 QLatin1String("/test/sharedTargetPath"),
                                                 QLatin1String("/test/sharedSshPath"),
                                                 QLatin1String("/test/sharedSrcPath"),
                                                 QLatin1String("/test/sharedConfigPath"),
                                                 QLatin1String("host"),QLatin1String("user"),
                                                 QLatin1String("/test/privateKey"),22,80,false,512,2,10000);

    map = AddMerSdkOperation::addSdk(map,
                                     QLatin1String("testSdk2"), true,
                                     QLatin1String("/test/sharedHomePath"),
                                     QLatin1String("/test/sharedTargetPath"),
                                     QLatin1String("/test/sharedSshPath"),
                                     QLatin1String("/test/sharedSrcPath"),
                                     QLatin1String("/test/sharedConfigPath"),
                                     QLatin1String("host"),QLatin1String("user"),
                                     QLatin1String("/test/privateKey"),22,80,false,512,2,10000);


    const QString sdk1 = QString::fromLatin1(Mer::Constants::MER_SDK_DATA_KEY) + QString::number(0);
    const QString sdk2 = QString::fromLatin1(Mer::Constants::MER_SDK_DATA_KEY) + QString::number(1);


    if (map.count() != 4
            || !map.contains(QLatin1String(Mer::Constants::MER_SDK_FILE_VERSION_KEY))
            || map.value(QLatin1String(Mer::Constants::MER_SDK_FILE_VERSION_KEY)).toInt() != 2
            || !map.contains(QLatin1String(Mer::Constants::MER_SDK_COUNT_KEY))
            || map.value(QLatin1String(Mer::Constants::MER_SDK_COUNT_KEY)).toInt() != 2
            || !map.contains(sdk1)
            || !map.contains(sdk2))
        return false;

    QVariantMap result = removeSdk(map, QLatin1String("testSdk2"));
    if (result.count() != 3
            || result.contains(sdk2)
            || !result.contains(sdk1)
            || !result.contains(QLatin1String(Mer::Constants::MER_SDK_COUNT_KEY))
            || result.value(QLatin1String(Mer::Constants::MER_SDK_COUNT_KEY)).toInt() != 1
            || !result.contains(QLatin1String(Mer::Constants::MER_SDK_FILE_VERSION_KEY))
            || result.value(QLatin1String(Mer::Constants::MER_SDK_FILE_VERSION_KEY)).toInt() != 2)
        return false;

    QString sdk = QString::fromLatin1(Mer::Constants::MER_SDK_DATA_KEY) + QString::number(0);
    QVariantMap sdkMap = result.value(sdk).toMap();
    if (sdkMap.value(QLatin1String(Mer::Constants::VM_NAME)).toString() == QLatin1String("testSdk2"))
        return false;

    result = removeSdk(map, QLatin1String("unknown"));
    if (result != map)
        return false;

    result = removeSdk(map, QLatin1String("testSdk"));
    if (result.count() != 3
              || result.contains(sdk2)
              || !result.contains(sdk1)
              || !result.contains(QLatin1String(Mer::Constants::MER_SDK_COUNT_KEY))
              || result.value(QLatin1String(Mer::Constants::MER_SDK_COUNT_KEY)).toInt() != 1
              || !result.contains(QLatin1String(Mer::Constants::MER_SDK_FILE_VERSION_KEY))
              || result.value(QLatin1String(Mer::Constants::MER_SDK_FILE_VERSION_KEY)).toInt() != 2)
        return false;

    sdk = QString::fromLatin1(Mer::Constants::MER_SDK_DATA_KEY) + QString::number(0);
    sdkMap = result.value(sdk).toMap();
    if (sdkMap.value(QLatin1String(Mer::Constants::VM_NAME)).toString() == QLatin1String("testSdk"))
        return false;


    return true;
}
#endif
