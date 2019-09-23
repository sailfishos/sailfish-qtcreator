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

#include "addsfdkdevicemodeloperation.h"
#include "addkeysoperation.h"
#include "findvalueoperation.h"
#include "rmkeysoperation.h"
#include "getoperation.h"
#include "sfdkutils.h"
#include "../../libs/sfdk/sfdkconstants.h"
#include <QRect>
#include <iostream>

const char MER_PARAM_NAME[] = "--name";
const char MER_PARAM_HRES_PX[] = "--hres-px";
const char MER_PARAM_VRES_PX[] = "--vres-px";
const char MER_PARAM_HSIZE_MM[] = "--hsize-mm";
const char MER_PARAM_VSIZE_MM[] = "--vsize-mm";
const char MER_PARAM_DCONF_DB[] = "--dconf-db";

namespace C = Sfdk::Constants;

AddSfdkDeviceModelOperation::AddSfdkDeviceModelOperation()
{
}

QString AddSfdkDeviceModelOperation::name() const
{
    return QLatin1String("addSfdkDeviceModel");
}

QString AddSfdkDeviceModelOperation::helpText() const
{
    return QLatin1String("add an Sfdk emulator device model");
}

QString AddSfdkDeviceModelOperation::argumentsHelpText() const
{
    const QString indent = QLatin1String("    ");
    return indent + QLatin1String(MER_PARAM_NAME) + QLatin1String(" <STRING>        name of this device model (required).\n")
         + indent + QLatin1String(MER_PARAM_HRES_PX) + QLatin1String(" <NUM>        display horizontal resolution in px (required).\n")
         + indent + QLatin1String(MER_PARAM_VRES_PX) + QLatin1String(" <NUM>        display vertical resolution in px (required).\n")
         + indent + QLatin1String(MER_PARAM_HSIZE_MM) + QLatin1String(" <NUM>       display horizontal size in milimeters (required).\n")
         + indent + QLatin1String(MER_PARAM_VSIZE_MM) + QLatin1String(" <NUM>       display vertical size in milimeters (required).\n")
         + indent + QLatin1String(MER_PARAM_DCONF_DB) + QLatin1String(" <STRING>    dconf bits specific to this device model (required).\n");
}

bool AddSfdkDeviceModelOperation::setArguments(const QStringList &args)
{

    for (int i = 0; i < args.count(); ++i) {
        const QString current = args.at(i);
        const QString next = ((i + 1) < args.count()) ? args.at(i + 1) : QString();

        if (current == QLatin1String(MER_PARAM_NAME)) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_name = next;
            continue;
        }

        if (current == QLatin1String(MER_PARAM_HRES_PX)) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_hres = next.toInt();
            continue;
        }

        if (current == QLatin1String(MER_PARAM_VRES_PX)) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_vres = next.toInt();
            continue;
        }

        if (current == QLatin1String(MER_PARAM_HSIZE_MM)) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_hsize = next.toInt();
            continue;
        }

        if (current == QLatin1String(MER_PARAM_VSIZE_MM)) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_vsize = next.toInt();
            continue;
        }

        if (current == QLatin1String(MER_PARAM_DCONF_DB)) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_dconfDb = next;
            continue;
        }
    }

    const char MISSING[] = " parameter missing.";
    bool error = false;
    if (m_name.isEmpty()) {
        std::cerr << MER_PARAM_NAME << MISSING << std::endl << std::endl;
        error = true;
    }
    if (m_hres == 0) {
        std::cerr << MER_PARAM_HRES_PX << MISSING << std::endl << std::endl;
        error = true;
    }
    if (m_vres == 0) {
        std::cerr << MER_PARAM_VRES_PX << MISSING << std::endl << std::endl;
        error = true;
    }
    if (m_hsize == 0) {
        std::cerr << MER_PARAM_HSIZE_MM << MISSING << std::endl << std::endl;
        error = true;
    }
    if (m_vsize == 0) {
        std::cerr << MER_PARAM_VSIZE_MM << MISSING << std::endl << std::endl;
        error = true;
    }
    if (m_dconfDb.isNull()) {
        std::cerr << MER_PARAM_DCONF_DB << MISSING << std::endl << std::endl;
        error = true;
    }

    return !error;
}

int AddSfdkDeviceModelOperation::execute() const
{
    useSfdkSettingsPath();

    QVariantMap map = load(QLatin1String("SfdkEmulators"));
    if (map.isEmpty())
        map = initializeDeviceModels();

    const QVariantMap result = addDeviceModel(map, m_name, m_hres, m_vres, m_hsize, m_vsize, m_dconfDb);

    if (result.isEmpty() || map == result)
        return 2;

    return save(result, QLatin1String("SfdkEmulators")) ? 0 : 3;
}

QVariantMap AddSfdkDeviceModelOperation::initializeDeviceModels()
{
    QVariantMap map;
    // Version and install dir should be initialized by AddSfdkEmulatorOperation
    map.insert(QLatin1String(C::EMULATORS_VERSION_KEY), -1);
    map.insert(QLatin1String(C::EMULATORS_INSTALL_DIR_KEY), QLatin1String("/dev/null"));
    map.insert(QLatin1String(C::EMULATORS_COUNT_KEY), 0);
    map.insert(QLatin1String(C::DEVICE_MODELS_COUNT_KEY), 0);
    return map;
}

QVariantMap AddSfdkDeviceModelOperation::addDeviceModel(const QVariantMap &map,
                                                       const QString &name,
                                                       int hres,
                                                       int vres,
                                                       int hsize,
                                                       int vsize,
                                                       const QString &dconfDb)
{
    QStringList valueKeys = FindValueOperation::findValue(map, QVariant(name));
    bool hasModel = false;
    foreach (const QString &t, valueKeys) {
        if (t.endsWith(QLatin1Char('/') + QLatin1String(C::DEVICE_MODEL_NAME))) {
            hasModel = true;
            break;
        }
    }
    if (hasModel) {
        std::cerr << "Error: " << qPrintable(name) << " device model already defined." << std::endl;
        return QVariantMap();
    }

    bool ok;
    const int count = GetOperation::get(map, QLatin1String(C::DEVICE_MODELS_COUNT_KEY)).toInt(&ok);
    if (!ok || count < 0) {
        std::cerr << "Error: Could not read device models count, file seems wrong." << std::endl;
        return QVariantMap();
    }

    const QString model = QString::fromLatin1(C::DEVICE_MODELS_DATA_KEY_PREFIX) + QString::number(count);

    // remove old count
    QVariantMap cleaned = RmKeysOperation::rmKeys(map, QStringList()
            << QLatin1String(C::DEVICE_MODELS_COUNT_KEY));

    KeyValuePairList data;
    auto addPrefix = [&](const QString &key, const QVariant &value) {
        return KeyValuePair(QStringList{model, key}, value);
    };
    data << addPrefix(QLatin1String(C::DEVICE_MODEL_NAME), QVariant(name));
    data << addPrefix(QLatin1String(C::DEVICE_MODEL_AUTODETECTED), QVariant(true));
    data << addPrefix(QLatin1String(C::DEVICE_MODEL_DISPLAY_RESOLUTION), QVariant(QRect(0, 0, hres, vres)));
    data << addPrefix(QLatin1String(C::DEVICE_MODEL_DISPLAY_SIZE), QVariant(QRect(0, 0, hsize, vsize)));
    data << addPrefix(QLatin1String(C::DEVICE_MODEL_DCONF), QVariant(dconfDb));
    data << KeyValuePair(QLatin1String(C::DEVICE_MODELS_COUNT_KEY), QVariant(count + 1));

    return AddKeysOperation::addKeys(cleaned, data);
}

#ifdef WITH_TESTS
bool AddSfdkDeviceModelOperation::test() const
{
    QVariantMap map = initializeDeviceModels();

    if (map.count() != 4
            || !map.contains(QLatin1String(C::EMULATORS_VERSION_KEY))
            || map.value(QLatin1String(C::EMULATORS_VERSION_KEY)).toInt() != -1
            || !map.contains(QLatin1String(C::EMULATORS_INSTALL_DIR_KEY))
            || map.value(QLatin1String(C::EMULATORS_INSTALL_DIR_KEY)).toString()
                != QLatin1String("/dev/null")
            || !map.contains(QLatin1String(C::EMULATORS_COUNT_KEY))
            || map.value(QLatin1String(C::EMULATORS_COUNT_KEY)).toInt() != 0
            || !map.contains(QLatin1String(C::DEVICE_MODELS_COUNT_KEY))
            || map.value(QLatin1String(C::DEVICE_MODELS_COUNT_KEY)).toInt() != 0)
        return false;

    map = addDeviceModel(map, QLatin1String("Test Device"), 500, 1000, 50, 100, QLatin1String("Test dconf"));

    const QString model = QString::fromLatin1(C::DEVICE_MODELS_DATA_KEY_PREFIX) + QString::number(0);

    if (map.count() != 5
            || !map.contains(QLatin1String(C::EMULATORS_VERSION_KEY))
            || map.value(QLatin1String(C::EMULATORS_VERSION_KEY)).toInt() != -1
            || !map.contains(QLatin1String(C::EMULATORS_INSTALL_DIR_KEY))
            || map.value(QLatin1String(C::EMULATORS_INSTALL_DIR_KEY)).toString()
                != QLatin1String("/dev/null")
            || !map.contains(QLatin1String(C::EMULATORS_COUNT_KEY))
            || map.value(QLatin1String(C::EMULATORS_COUNT_KEY)).toInt() != 0
            || !map.contains(QLatin1String(C::DEVICE_MODELS_COUNT_KEY))
            || map.value(QLatin1String(C::DEVICE_MODELS_COUNT_KEY)).toInt() != 1
            || !map.contains(model))
        return false;

    QVariantMap modelMap = map.value(model).toMap();
    if (modelMap.count() != 5
            || !modelMap.contains(QLatin1String(C::DEVICE_MODEL_NAME))
            || modelMap.value(QLatin1String(C::DEVICE_MODEL_NAME)).toString() != QLatin1String("Test Device")
            || !modelMap.contains(QLatin1String(C::DEVICE_MODEL_AUTODETECTED))
            || modelMap.value(QLatin1String(C::DEVICE_MODEL_AUTODETECTED)).toBool() != true
            || !modelMap.contains(QLatin1String(C::DEVICE_MODEL_DISPLAY_RESOLUTION))
            || modelMap.value(QLatin1String(C::DEVICE_MODEL_DISPLAY_RESOLUTION)).toRect() != QRect(0, 0, 500, 1000)
            || !modelMap.contains(QLatin1String(C::DEVICE_MODEL_DISPLAY_SIZE))
            || modelMap.value(QLatin1String(C::DEVICE_MODEL_DISPLAY_SIZE)).toRect() != QRect(0, 0, 50, 100)
            || !modelMap.contains(QLatin1String(C::DEVICE_MODEL_DCONF))
            || modelMap.value(QLatin1String(C::DEVICE_MODEL_DCONF)).toString() != QLatin1String("Test dconf"))
        return false;

    return true;
}
#endif
