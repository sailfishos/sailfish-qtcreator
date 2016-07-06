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

#include "addmerdevicemodeloperation.h"
#include "addkeysoperation.h"
#include "findvalueoperation.h"
#include "rmkeysoperation.h"
#include "getoperation.h"
#include "../../plugins/mer/merconstants.h"
#include <QRect>
#include <iostream>

const char MER_PARAM_NAME[] = "--name";
const char MER_PARAM_HRES_PX[] = "--hres-px";
const char MER_PARAM_VRES_PX[] = "--vres-px";
const char MER_PARAM_HSIZE_MM[] = "--hsize-mm";
const char MER_PARAM_VSIZE_MM[] = "--vsize-mm";

AddMerDeviceModelOperation::AddMerDeviceModelOperation()
{
}

QString AddMerDeviceModelOperation::name() const
{
    return QLatin1String("addMerDeviceModel");
}

QString AddMerDeviceModelOperation::helpText() const
{
    return QLatin1String("add Mer emulator device model to Qt Creator configuration");
}

QString AddMerDeviceModelOperation::argumentsHelpText() const
{
    const QString indent = QLatin1String("    ");
    return indent + QLatin1String(MER_PARAM_NAME) + QLatin1String(" <STRING>        name of this device model (required).\n")
         + indent + QLatin1String(MER_PARAM_HRES_PX) + QLatin1String(" <NUM>        display horizontal resolution in px (required).\n")
         + indent + QLatin1String(MER_PARAM_VRES_PX) + QLatin1String(" <NUM>        display vertical resolution in px (required).\n")
         + indent + QLatin1String(MER_PARAM_HSIZE_MM) + QLatin1String(" <NUM>       display horizontal size in milimeters (required).\n")
         + indent + QLatin1String(MER_PARAM_VSIZE_MM) + QLatin1String(" <NUM>       display vertical size in milimeters (required).\n");
}

bool AddMerDeviceModelOperation::setArguments(const QStringList &args)
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

    return !error;
}

int AddMerDeviceModelOperation::execute() const
{
    QVariantMap map = load(QLatin1String("mersdk-device-models"));
    if (map.isEmpty())
        map = initializeDeviceModels();

    const QVariantMap result = addDeviceModel(map, m_name, m_hres, m_vres, m_hsize, m_vsize);

    if (result.isEmpty() || map == result)
        return 2;

    return save(result, QLatin1String("mersdk-device-models")) ? 0 : 3;
}

QVariantMap AddMerDeviceModelOperation::initializeDeviceModels()
{
    QVariantMap map;
    map.insert(QLatin1String(Mer::Constants::MER_DEVICE_MODELS_FILE_VERSION_KEY), 1);
    map.insert(QLatin1String(Mer::Constants::MER_DEVICE_MODELS_COUNT_KEY), 0);
    return map;
}

QVariantMap AddMerDeviceModelOperation::addDeviceModel(const QVariantMap &map,
                                                       const QString &name,
                                                       int hres,
                                                       int vres,
                                                       int hsize,
                                                       int vsize)
{
    QStringList valueKeys = FindValueOperation::findValue(map, QVariant(name));
    bool hasModel = false;
    foreach (const QString &t, valueKeys) {
        if (t.endsWith(QLatin1Char('/') + QLatin1String(Mer::Constants::MER_DEVICE_MODEL_NAME))) {
            hasModel = true;
            break;
        }
    }
    if (hasModel) {
        std::cerr << "Error: " << qPrintable(name) << " device model already defined." << std::endl;
        return QVariantMap();
    }

    bool ok;
    const int count = GetOperation::get(map, QLatin1String(Mer::Constants::MER_DEVICE_MODELS_COUNT_KEY)).toInt(&ok);
    if (!ok || count < 0) {
        std::cerr << "Error: Could not read device models count, file seems wrong." << std::endl;
        return QVariantMap();
    }

    const QString model = QString::fromLatin1(Mer::Constants::MER_DEVICE_MODELS_DATA_KEY) + QString::number(count);

    // remove old count
    QVariantMap cleaned = RmKeysOperation::rmKeys(map,  QStringList() << QLatin1String(Mer::Constants::MER_DEVICE_MODELS_COUNT_KEY));

    KeyValuePairList data;
    data << KeyValuePair(QStringList() << model << QLatin1String(Mer::Constants::MER_DEVICE_MODEL_NAME), QVariant(name));
    data << KeyValuePair(QStringList() << model << QLatin1String(Mer::Constants::MER_DEVICE_MODEL_DISPLAY_RESOLUTION),
                         QVariant(QRect(0, 0, hres, vres)));
    data << KeyValuePair(QStringList() << model << QLatin1String(Mer::Constants::MER_DEVICE_MODEL_DISPLAY_SIZE),
                         QVariant(QRect(0, 0, hsize, vsize)));
    data << KeyValuePair(QStringList() << QLatin1String(Mer::Constants::MER_DEVICE_MODELS_COUNT_KEY), QVariant(count + 1));

    return AddKeysOperation::addKeys(cleaned, data);
}

#ifdef WITH_TESTS
bool AddMerDeviceModelOperation::test() const
{
    QVariantMap map = initializeDeviceModels();

    if (map.count() != 2
            || !map.contains(QLatin1String(Mer::Constants::MER_DEVICE_MODELS_FILE_VERSION_KEY))
            || map.value(QLatin1String(Mer::Constants::MER_DEVICE_MODELS_FILE_VERSION_KEY)).toInt() != 1
            || !map.contains(QLatin1String(Mer::Constants::MER_DEVICE_MODELS_COUNT_KEY))
            || map.value(QLatin1String(Mer::Constants::MER_DEVICE_MODELS_COUNT_KEY)).toInt() != 0)
        return false;

    map = addDeviceModel(map, QLatin1String("Test Device"), 500, 1000, 50, 100);

    const QString model = QString::fromLatin1(Mer::Constants::MER_DEVICE_MODELS_DATA_KEY) + QString::number(0);

    if (map.count() != 3
            || !map.contains(QLatin1String(Mer::Constants::MER_DEVICE_MODELS_FILE_VERSION_KEY))
            || map.value(QLatin1String(Mer::Constants::MER_DEVICE_MODELS_FILE_VERSION_KEY)).toInt() != 1
            || !map.contains(QLatin1String(Mer::Constants::MER_DEVICE_MODELS_COUNT_KEY))
            || map.value(QLatin1String(Mer::Constants::MER_DEVICE_MODELS_COUNT_KEY)).toInt() != 1
            || !map.contains(model))
        return false;

    QVariantMap modelMap= map.value(model).toMap();
    if (modelMap.count() != 13
            || !modelMap.contains(QLatin1String(Mer::Constants::MER_DEVICE_MODEL_NAME))
            || modelMap.value(QLatin1String(Mer::Constants::MER_DEVICE_MODEL_NAME)).toString() != QLatin1String("Test Device")
            || !modelMap.contains(QLatin1String(Mer::Constants::MER_DEVICE_MODEL_DISPLAY_RESOLUTION))
            || modelMap.value(QLatin1String(Mer::Constants::MER_DEVICE_MODEL_DISPLAY_RESOLUTION)).toSize() != QSize(500, 1000)
            || !modelMap.contains(QLatin1String(Mer::Constants::MER_DEVICE_MODEL_DISPLAY_SIZE))
            || modelMap.value(QLatin1String(Mer::Constants::MER_DEVICE_MODEL_DISPLAY_SIZE)).toSize() != QSize(50, 100))
        return false;

    return true;
}
#endif
