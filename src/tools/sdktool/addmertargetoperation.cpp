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

#include "addmertargetoperation.h"
#include "../../plugins/mer/merconstants.h"
#include "../../plugins/mer/mertargetsxmlparser.h"
#include "utils/persistentsettings.h"
#include "addkeysoperation.h"
#include "findvalueoperation.h"
#include "rmkeysoperation.h"
#include "getoperation.h"
#include <QDir>
#include <iostream>

const char MER_PARAM_MER_TARGETS_DIR[] = "--mer-targets-dir";
const char MER_PARAM_TARGET_NAME[] = "--target-name";
const char MER_PARAM_QMAKE_QUERY[] = "--qmake-query";
const char MER_PARAM_GCC_DUMPMACHINE[] = "--gcc-dumpmachine";
const char MER_PARAM_RPMVALIDATION_SUITES[] = "--rpmvalidation-suites";

AddMerTargetOperation::AddMerTargetOperation()
{
}

QString AddMerTargetOperation::name() const
{
    return QLatin1String("addMerTarget");
}

QString AddMerTargetOperation::helpText() const
{
    return QLatin1String("add mer target to Qt Creator configuration");
}

QString AddMerTargetOperation::argumentsHelpText() const
{
    const QString indent = QLatin1String("    ");
    return indent + QLatin1String(MER_PARAM_MER_TARGETS_DIR) + QLatin1String(" <PATH>   shared \"targets\" folder (required).\n")
         + indent + QLatin1String(MER_PARAM_TARGET_NAME) + QLatin1String(" <NAME>       display name (required).\n")
         + indent + QLatin1String(MER_PARAM_QMAKE_QUERY) + QLatin1String(" <FILE>       'qmake -query' dump (required).\n")
         + indent + QLatin1String(MER_PARAM_GCC_DUMPMACHINE) + QLatin1String(" <FILE>   'gcc --dumpmachine' dump (required).\n")
         + indent + QLatin1String(MER_PARAM_RPMVALIDATION_SUITES) + QLatin1String(" <FILE>   'rpmvalidation --list-suites <target>' dump (required).\n");
}

bool AddMerTargetOperation::setArguments(const QStringList &args)
{

    for (int i = 0; i < args.count(); ++i) {
        const QString current = args.at(i);
        const QString next = ((i + 1) < args.count()) ? args.at(i + 1) : QString();

        if (current == QLatin1String(MER_PARAM_MER_TARGETS_DIR)) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_targetsDir = next;
            continue;
        }

        if (current == QLatin1String(MER_PARAM_TARGET_NAME)) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_targetName = next;
            continue;
        }

        if (current == QLatin1String(MER_PARAM_QMAKE_QUERY)) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_qmakeQueryFileName = next;
            continue;
        }

        if (current == QLatin1String(MER_PARAM_GCC_DUMPMACHINE)) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_gccDumpmachineFileName = next;
            continue;
        }

        if (current == QLatin1String(MER_PARAM_RPMVALIDATION_SUITES)) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_rpmValidationSuitesFileName = next;
            continue;
        }
    }

    const char MISSING[] = " parameter missing.";
    if (m_targetsDir.isEmpty())
        std::cerr << MER_PARAM_MER_TARGETS_DIR << MISSING << std::endl << std::endl;
    if (m_targetName.isEmpty())
        std::cerr << MER_PARAM_TARGET_NAME << MISSING << std::endl << std::endl;
    if (m_qmakeQueryFileName.isEmpty())
        std::cerr << MER_PARAM_QMAKE_QUERY << MISSING << std::endl << std::endl;
    if (m_gccDumpmachineFileName.isEmpty())
        std::cerr << MER_PARAM_GCC_DUMPMACHINE << MISSING << std::endl << std::endl;
    if (m_rpmValidationSuitesFileName.isEmpty())
        std::cerr << MER_PARAM_RPMVALIDATION_SUITES << MISSING << std::endl << std::endl;

    return !m_targetsDir.isEmpty() && !m_targetName.isEmpty() && !m_qmakeQueryFileName.isEmpty() && !m_gccDumpmachineFileName.isEmpty()
        && !m_rpmValidationSuitesFileName.isEmpty();
}

int AddMerTargetOperation::execute() const
{
    QVariantMap map = load(m_targetsDir);
    if (map.isEmpty())
        map = initializeTargets();

    const QVariantMap result = addTarget(map, m_targetName, m_qmakeQueryFileName, m_gccDumpmachineFileName,
            m_rpmValidationSuitesFileName);

    if (result.isEmpty() || map == result)
        return 2;

    return save(result, m_targetsDir) ? 0 : 3;
}

QVariantMap AddMerTargetOperation::initializeTargets()
{
    QVariantMap map;
    map.insert(QLatin1String(Mer::Constants::MER_TARGET_FILE_VERSION_KEY), 2);
    return map;
}

QString AddMerTargetOperation::readCacheFile(const QString &cacheFile)
{
    QByteArray read;
    QFile file(cacheFile);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return QString();

    while (!file.atEnd()) {
        QByteArray line = file.readLine();
        read.append(line);
    }

    file.close();

    return QLatin1String(read);
}

QVariantMap AddMerTargetOperation::addTarget(const QVariantMap &map, const QString &name, const QString &qmakeFileName,
                                             const QString &gccFileName, const QString &rpmValidationSuitesFileName)
{
    bool hasTarget = false;
    QVariantList targetData = map.value(QLatin1String(Mer::Constants::MER_TARGET_KEY)).toList();
    foreach (const QVariant &data, targetData) {
        Mer::MerTargetData d = data.value<Mer::MerTargetData>();
        hasTarget = d.name == name;
        if (hasTarget)
            break;
    }

    if (hasTarget) {
        std::cerr << "Error: Name " << qPrintable(name) << " already defined as target." << std::endl;
        return QVariantMap();
    }

    const int count = targetData.count();
    if (count < 0) {
        std::cerr << "Error: Count found targets, file seems wrong." << std::endl;
        return QVariantMap();
    }

    Mer::MerTargetData newTarget;
    newTarget.name = name;

    const QString qmake = readCacheFile(qmakeFileName);
    if (qmake.isEmpty()){
        std::cerr << "Error: Could not read file " << qPrintable(qmakeFileName) << std::endl;
        return QVariantMap();
    }
    newTarget.qmakeQuery = qmake;

    const QString gcc = readCacheFile(gccFileName);
    if (gcc.isEmpty()) {
        std::cerr << "Error: Could not read file " << qPrintable(gccFileName) << std::endl;
        return QVariantMap();
    }
    newTarget.gccDumpMachine = gcc;

    const QString rpm = readCacheFile(rpmValidationSuitesFileName);
    if (rpm.isEmpty()) {
        std::cerr << "Error: Could not read file " << qPrintable(rpmValidationSuitesFileName) << std::endl;
        return QVariantMap();
    }
    newTarget.rpmValidationSuites = rpm;

    QVariantMap newMap = map;
    targetData.append(QVariant::fromValue(newTarget));
    newMap.insert(QLatin1String(Mer::Constants::MER_TARGET_KEY), targetData);

    return newMap;
}

QVariantMap AddMerTargetOperation::load(const QString &root)
{
    QVariantMap map;

    // Read values from original file:
    const Utils::FileName path = Utils::FileName::fromString(root + QLatin1String(Mer::Constants::MER_TARGETS_FILENAME));
    if (path.toFileInfo().exists()) {
        Mer::MerTargetsXmlReader reader(path.toString());
        if (reader.hasError()) {
            std::cerr << "Error: Could not read file " << qPrintable(path.toString())
                      << ": " << qPrintable(reader.errorString()) << std::endl;
            return QVariantMap();
        }
        const QList<Mer::MerTargetData> targetData = reader.targetData();
        QVariantList data;
        foreach (const Mer::MerTargetData &td, targetData)
            data.append(QVariant::fromValue(td));
        map.insert(QLatin1String(Mer::Constants::MER_TARGET_FILE_VERSION_KEY), reader.version());
        map.insert(QLatin1String(Mer::Constants::MER_TARGET_KEY), data);
    }
    return map;
}

bool AddMerTargetOperation::save(const QVariantMap &map, const QString &root)
{
    const Utils::FileName path = Utils::FileName::fromString(root + QLatin1String(Mer::Constants::MER_TARGETS_FILENAME));

    if (path.isEmpty()) {
        std::cerr << "Error: No path found for " << qPrintable(root) << "." << std::endl;
        return false;
    }

    const int version = map.value(QLatin1String(Mer::Constants::MER_TARGET_FILE_VERSION_KEY)).toInt();
    const QVariantList targetData = map.value(QLatin1String(Mer::Constants::MER_TARGET_KEY)).toList();
    QList<Mer::MerTargetData> data;
    foreach (const QVariant &td, targetData)
        data.append(td.value<Mer::MerTargetData>());
    Mer::MerTargetsXmlWriter writer(path.toString(), version, data);
    return !writer.hasError() && QFile::setPermissions(path.toString(),
                                                       QFile::ReadOwner | QFile::WriteOwner
                                                       | QFile::ReadGroup | QFile::ReadOther);
}
