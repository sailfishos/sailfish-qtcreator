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
#include "utils/persistentsettings.h"
#include "addkeysoperation.h"
#include "findvalueoperation.h"
#include "rmkeysoperation.h"
#include "getoperation.h"
#include <QDir>
#include <iostream>

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
    return QLatin1String("    --sdk <PATH>                               mer sdk root (required).\n"
                         "    --name <NAME>                              display name of the new target (required).\n"
                         "    --qmake <FILE>                             qmake dump used for new target (required).\n"
                         "    --gcc <FILE>                               gcc dump used for new traget (required).\n");
}

bool AddMerTargetOperation::setArguments(const QStringList &args)
{

    for (int i = 0; i < args.count(); ++i) {
        const QString current = args.at(i);
        const QString next = ((i + 1) < args.count()) ? args.at(i + 1) : QString();

        if (current == QLatin1String("--sdk")) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_sdkRoot = next;
            continue;
        }

        if (current == QLatin1String("--name")) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_name = next;
            continue;
        }

        if (current == QLatin1String("--qmake")) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_qmakeDumpFileName = next;
            continue;
        }

        if (current == QLatin1String("--gcc")) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_gccDumpFileName = next;
            continue;
        }
    }

    if (m_sdkRoot.isEmpty())
        std::cerr << "No sdk root given for target." << std::endl << std::endl;
    if (m_name.isEmpty())
        std::cerr << "No name given for target." << std::endl << std::endl;
    if (m_qmakeDumpFileName.isEmpty())
        std::cerr << "No qmake dump file given for target." << std::endl << std::endl;
    if (m_gccDumpFileName.isEmpty())
        std::cerr << "No gcc dump given for target." << std::endl << std::endl;

    return !m_sdkRoot.isEmpty() && !m_name.isEmpty() && !m_qmakeDumpFileName.isEmpty() && !m_gccDumpFileName.isEmpty();
}

int AddMerTargetOperation::execute() const
{
    QVariantMap map = load(m_sdkRoot);
    if (map.isEmpty())
        map = initializeTargets();

    const QVariantMap result = addTarget(map, m_name, m_qmakeDumpFileName, m_gccDumpFileName);

    if (result.isEmpty() || map == result)
        return -2;

    return save(result, m_sdkRoot) ? 0 : -3;
}

QVariantMap AddMerTargetOperation::initializeTargets() const
{
    QVariantMap map;
    map.insert(QLatin1String(Mer::Constants::MER_TARGET_FILE_VERSION_KEY), 2);
    map.insert(QLatin1String(Mer::Constants::MER_TARGET_COUNT_KEY), 0);
    return map;
}

QString AddMerTargetOperation::readCacheFile(const QString &cacheFile) const
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
                                             const QString &gccFileName) const
{
    QStringList valueKeys = FindValueOperation::findValues(map, QVariant(name));
    bool hasTarget = false;
    foreach (const QString &t, valueKeys) {
        if (t.endsWith(QString(QLatin1Char('/')) + QLatin1String(Mer::Constants::TARGET_NAME))) {
            hasTarget = true;
            break;
        }
    }
    if (hasTarget) {
        std::cerr << "Error: Name " << qPrintable(name) << " already defined as target." << std::endl;
        return QVariantMap();
    }

    bool ok;
    int count = GetOperation::get(map, QLatin1String(Mer::Constants::MER_TARGET_COUNT_KEY)).toInt(&ok);
    if (!ok || count < 0) {
        std::cerr << "Error: Count found targets, file seems wrong." << std::endl;
        return QVariantMap();
    }

    QString qmake = readCacheFile(qmakeFileName);
    if (qmake.isEmpty()){
        std::cerr << "Error: Could not read file " << qPrintable(qmakeFileName) << std::endl;
        return QVariantMap();
    }

    QString gcc = readCacheFile(gccFileName);
    if(gcc.isEmpty()) {
        std::cerr << "Error: Could not read file " << qPrintable(gccFileName) << std::endl;
        return QVariantMap();
    }

    const QString target = QString::fromLatin1(Mer::Constants::MER_TARGET_DATA_KEY) + QString::number(count);

    // remove old count

    QVariantMap cleaned = RmKeysOperation::rmKeys(map,  QStringList() << QLatin1String(Mer::Constants::MER_TARGET_COUNT_KEY));

    KeyValuePairList data;
    data << KeyValuePair(QStringList() << target << QLatin1String(Mer::Constants::TARGET_NAME), QVariant(name));
    data << KeyValuePair(QStringList() << target << QLatin1String(Mer::Constants::QMAKE_DUMP), QVariant(qmake));
    data << KeyValuePair(QStringList() << target << QLatin1String(Mer::Constants::GCC_DUMP), QVariant(gcc));
    data << KeyValuePair(QStringList() << QLatin1String(Mer::Constants::MER_TARGET_COUNT_KEY), QVariant(count + 1));

    return AddKeysOperation::addKeys(cleaned, data);
}

QVariantMap AddMerTargetOperation::load(const QString &root) const
{
    QVariantMap map;

    // Read values from original file:
    Utils::FileName path = Utils::FileName::fromString(root + QLatin1String("/targets/targets.xml"));
    if (path.toFileInfo().exists()) {
        Utils::PersistentSettingsReader reader;
        if (!reader.load(path))
            return QVariantMap();
        map = reader.restoreValues();
    }
    return map;
}

bool AddMerTargetOperation::save(const QVariantMap &map, const QString &root) const
{
    Utils::FileName path = Utils::FileName::fromString(root + QLatin1String("/targets/targets.xml"));

    if (path.isEmpty()) {
        std::cerr << "Error: No path found for " << qPrintable(root) << "." << std::endl;
        return false;
    }

    Utils::FileName dir = path.parentDir();
    if (!dir.toFileInfo().exists())
        QDir(root).mkpath(dir.toString());

    Utils::PersistentSettingsWriter writer(path, QLatin1String("unknown"));
    return writer.save(map, 0)
            && QFile::setPermissions(path.toString(),
                                     QFile::ReadOwner | QFile::WriteOwner
                                     | QFile::ReadGroup | QFile::ReadOther);
}
