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

#include "rmsfdkbuildengineoperation.h"
#include "addsfdkbuildengineoperation.h"
#include "getoperation.h"
#include "rmkeysoperation.h"
#include "addkeysoperation.h"
#include "sfdkutils.h"
#include "../../libs/sfdk/sfdkconstants.h"
#include <iostream>

namespace C = Sfdk::Constants;

RmSfdkBuildEngineOperation::RmSfdkBuildEngineOperation()
{
}

QString RmSfdkBuildEngineOperation::name() const
{
    return QLatin1String("rmSfdkBuildEngine");
}

QString RmSfdkBuildEngineOperation::helpText() const
{
    return QLatin1String("remove an Sfdk build engine");
}

QString RmSfdkBuildEngineOperation::argumentsHelpText() const
{
    return QLatin1String("    --vm-uri <URI>                build engine virtual machine URI (required).\n");
}

bool RmSfdkBuildEngineOperation::setArguments(const QStringList &args)
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

int RmSfdkBuildEngineOperation::execute() const
{
    useSfdkSettingsPath();

    QVariantMap map = load(QLatin1String("SfdkBuildEngines"));
    if (map.isEmpty())
        return 2;

    QVariantMap result = removeBuildEngine(map, m_vmUri);

    if (result == map)
        return 2;

    return save(result, QLatin1String("SfdkBuildEngines")) ? 0 : 3;
}

QVariantMap RmSfdkBuildEngineOperation::removeBuildEngine(const QVariantMap &map, const QUrl &vmUri)
{
    QVariantList engineList;
    bool ok;
    int version = GetOperation::get(map, QLatin1String(C::BUILD_ENGINES_VERSION_KEY)).toInt(&ok);
    if (!ok) {
        std::cerr << "Error: The version found in map is not an integer." << std::endl;
        return map;
    }
    QString installDir = GetOperation::get(map, QLatin1String(C::BUILD_ENGINES_INSTALL_DIR_KEY)).toString();
    if (installDir.isEmpty()) {
        std::cerr << "Error: No install-directory found in map or empty." << std::endl;
        return map;
    }
    int count = GetOperation::get(map, QLatin1String(C::BUILD_ENGINES_COUNT_KEY)).toInt(&ok);
    if (!ok) {
        std::cerr << "Error: The count found in map is not an integer." << std::endl;
        return map;
    }

    bool removed = false;
    for (int i = 0; i < count; ++i) {
        const QString engine = QString::fromLatin1(C::BUILD_ENGINES_DATA_KEY_PREFIX) + QString::number(i);
        QVariantMap engineMap = map.value(engine).toMap();
        if (engineMap.value(QLatin1String(C::BUILD_ENGINE_VM_URI)).toUrl() == vmUri) {
            removed = true;
            continue;
        }
        engineList << engineMap;
    }
    if (!removed) {
        std::cerr << "Error: Build engine was not found." << std::endl;
        return map;
    }

    // insert data:
    KeyValuePairList data;
    data << KeyValuePair(QStringList() << QLatin1String(C::BUILD_ENGINES_VERSION_KEY), QVariant(version));
    data << KeyValuePair(QStringList() << QLatin1String(C::BUILD_ENGINES_INSTALL_DIR_KEY), QVariant(installDir));
    data << KeyValuePair(QStringList() << QLatin1String(C::BUILD_ENGINES_COUNT_KEY), QVariant(count - 1));

    for (int i = 0; i < engineList.count(); ++i) {
        data << KeyValuePair(QStringList()
                << QString::fromLatin1(C::BUILD_ENGINES_DATA_KEY_PREFIX) + QString::number(i),
                engineList.at(i));
    }

    QVariantMap result;
    return AddKeysOperation::addKeys(result, data);
}

#ifdef WITH_TESTS
bool RmSfdkBuildEngineOperation::test() const
{
    QVariantMap map = AddSfdkBuildEngineOperation::initializeBuildEngines(2, QLatin1String("/dir"));
    map = AddSfdkBuildEngineOperation::addBuildEngine(map,
                                                 QUrl("sfdkvm:VirtualBox#testBuildEngine"), true,
                                                 QLatin1String("/test/sharedHomePath"),
                                                 QLatin1String("/test/sharedTargetPath"),
                                                 QLatin1String("/test/sharedSshPath"),
                                                 QLatin1String("/test/sharedSrcPath"),
                                                 QLatin1String("/test/sharedConfigPath"),
                                                 QLatin1String("host"),QLatin1String("user"),
                                                 QLatin1String("/test/privateKey"),22,80,false);

    map = AddSfdkBuildEngineOperation::addBuildEngine(map,
                                     QUrl("sfdkvm:VirtualBox#testBuildEngine2"), true,
                                     QLatin1String("/test/sharedHomePath"),
                                     QLatin1String("/test/sharedTargetPath"),
                                     QLatin1String("/test/sharedSshPath"),
                                     QLatin1String("/test/sharedSrcPath"),
                                     QLatin1String("/test/sharedConfigPath"),
                                     QLatin1String("host"),QLatin1String("user"),
                                     QLatin1String("/test/privateKey"),22,80,false);

    const QString engine1 = QString::fromLatin1(C::BUILD_ENGINES_DATA_KEY_PREFIX) + QString::number(0);
    const QString engine2 = QString::fromLatin1(C::BUILD_ENGINES_DATA_KEY_PREFIX) + QString::number(1);

    if (map.count() != 5
            || !map.contains(QLatin1String(C::BUILD_ENGINES_VERSION_KEY))
            || map.value(QLatin1String(C::BUILD_ENGINES_VERSION_KEY)).toInt() != 2
            || !map.contains(QLatin1String(C::BUILD_ENGINES_INSTALL_DIR_KEY))
            || map.value(QLatin1String(C::BUILD_ENGINES_INSTALL_DIR_KEY)).toString()
                != QLatin1String("/dir")
            || !map.contains(QLatin1String(C::BUILD_ENGINES_COUNT_KEY))
            || map.value(QLatin1String(C::BUILD_ENGINES_COUNT_KEY)).toInt() != 2
            || !map.contains(engine1)
            || !map.contains(engine2))
        return false;
    QVariantMap result = removeBuildEngine(map, QUrl("sfdkvm:VirtualBox#testBuildEngine2"));
    if (result.count() != 4
            || result.contains(engine2)
            || !result.contains(engine1)
            || !result.contains(QLatin1String(C::BUILD_ENGINES_COUNT_KEY))
            || result.value(QLatin1String(C::BUILD_ENGINES_COUNT_KEY)).toInt() != 1
            || !result.contains(QLatin1String(C::BUILD_ENGINES_VERSION_KEY))
            || result.value(QLatin1String(C::BUILD_ENGINES_VERSION_KEY)).toInt() != 2
            || !map.contains(QLatin1String(C::BUILD_ENGINES_INSTALL_DIR_KEY))
            || map.value(QLatin1String(C::BUILD_ENGINES_INSTALL_DIR_KEY)).toString()
                != QLatin1String("/dir"))
        return false;

    QString engine = QString::fromLatin1(C::BUILD_ENGINES_DATA_KEY_PREFIX) + QString::number(0);
    QVariantMap engineMap = result.value(engine).toMap();
    if (engineMap.value(QLatin1String(C::BUILD_ENGINE_VM_URI)).toUrl() == QUrl("sfdkvm:VirtualBox#testBuildEngine2"))
        return false;

    result = removeBuildEngine(map, QUrl("sfdkvm:VirtualBox#unknown"));
    if (result != map)
        return false;

    result = removeBuildEngine(map, QUrl("sfdkvm:VirtualBox#testBuildEngine"));
    if (result.count() != 4
              || result.contains(engine2)
              || !result.contains(engine1)
              || !result.contains(QLatin1String(C::BUILD_ENGINES_COUNT_KEY))
              || result.value(QLatin1String(C::BUILD_ENGINES_COUNT_KEY)).toInt() != 1
              || !result.contains(QLatin1String(C::BUILD_ENGINES_VERSION_KEY))
              || result.value(QLatin1String(C::BUILD_ENGINES_VERSION_KEY)).toInt() != 2
              || !map.contains(QLatin1String(C::BUILD_ENGINES_INSTALL_DIR_KEY))
              || map.value(QLatin1String(C::BUILD_ENGINES_INSTALL_DIR_KEY)).toString()
                  != QLatin1String("/dir"))
        return false;

    engine = QString::fromLatin1(C::BUILD_ENGINES_DATA_KEY_PREFIX) + QString::number(0);
    engineMap = result.value(engine).toMap();
    if (engineMap.value(QLatin1String(C::BUILD_ENGINE_VM_URI)).toUrl() == QUrl("tsfdkvm:VirtualBox#estBuildEngine"))
        return false;

    return true;
}
#endif
