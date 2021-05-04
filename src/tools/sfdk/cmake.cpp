/****************************************************************************
**
** Copyright (C) 2021 Jolla Ltd.
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

#include "cmake.h"

#include "sdkmanager.h"
#include "sfdkconstants.h"
#include "sfdkglobal.h"

#include <sfdk/buildengine.h>
#include <sfdk/sfdkconstants.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QDirIterator>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

using namespace Sfdk;
using namespace Utils;

namespace Sfdk {

namespace {

const char CMAKE_FILE_API_RAW_PREFIX[] = ".sfdk/cmake";
const char CMAKE_FILE_API_RAW_QUERY[] = ".sfdk/cmake/api/v1/query";
const char CMAKE_FILE_API_RAW_REPLY[] = ".sfdk/cmake/api/v1/reply";
const char CMAKE_FILE_API_QUERY[] = ".cmake/api/v1/query";
const char CMAKE_FILE_API_REPLY[] = ".cmake/api/v1/reply";

} // namespace anonymous

void CMakeHelper::maybePrepareCMakeApiPathMapping(QProcessEnvironment *extraEnvironment)
{
    if (!QFile::exists(CMAKE_FILE_API_QUERY))
        return;

    QDir queryDir(CMAKE_FILE_API_QUERY);
    QDir rawQueryDir(CMAKE_FILE_API_RAW_QUERY);

    if (!rawQueryDir.mkpath(".")) {
        qCCritical(sfdk) << "Failed to create raw CMake query directory" << rawQueryDir;
        return;
    }

    extraEnvironment->insert("CMAKE_FILE_API_PREFIX", CMAKE_FILE_API_RAW_PREFIX);

    const QSet<QString> entries = queryDir.entryList(QDir::Files | QDir::NoDotAndDotDot).toSet();

    // Copy current entries
    for (const QString &entry : entries) {
        rawQueryDir.remove(entry);
        if (!QFile::copy(queryDir.filePath(entry), rawQueryDir.filePath(entry))) {
            qCCritical(sfdk) << "Failed to copy" << entry << "into" << rawQueryDir.path();
            return;
        }
    }

    // Remove extra entries
    for (const QString &entry : rawQueryDir.entryList(QDir::Files | QDir::NoDotAndDotDot)) {
        if (queryDir.exists(entry))
            continue;
        if (!rawQueryDir.remove(entry)) {
            qCCritical(sfdk) << "Failed to remove" << entry << "from" << rawQueryDir.path();
            return;
        }
    }
}

void CMakeHelper::maybeDoCMakeApiPathMapping()
{
    if (!QFile::exists(CMAKE_FILE_API_QUERY))
        return;

    QString errorMessage;

    const BuildTargetData target = SdkManager::configuredTarget(&errorMessage);
    if (!target.isValid()) {
        qCDebug(sfdk) << "No build target configured - skipping CMake path mapping";
        return;
    }

    QTC_CHECK(!target.sysRoot.toString().contains('\\'));
    QTC_CHECK(!target.sysRoot.toString().endsWith('/'));
    QTC_CHECK(!target.toolsPath.toString().contains('\\'));
    QTC_CHECK(!target.toolsPath.toString().endsWith('/'));

    QDir replyDir(CMAKE_FILE_API_REPLY);
    QDir rawReplyDir(CMAKE_FILE_API_RAW_REPLY);

    if (!replyDir.mkpath(".")) {
        qCCritical(sfdk) << "Failed to create CMake reply directory" << replyDir;
        return;
    }

    const QSet<QString> entries = rawReplyDir.entryList(QDir::Files | QDir::NoDotAndDotDot).toSet();

    // Copy non-index entries first
    for (const QString &entry : entries) {
        if (entry.startsWith("index-"))
            continue;
        if (!doCMakeApiReplyPathMapping(rawReplyDir, replyDir, entry, target))
            return;
    }

    // Copy index entries last
    for (const QString &entry : entries) {
        if (!entry.startsWith("index-"))
            continue;
        if (!doCMakeApiReplyPathMapping(rawReplyDir, replyDir, entry, target))
            return;
    }

    // Remove extra entries
    for (const QString &entry : replyDir.entryList(QDir::Files | QDir::NoDotAndDotDot)) {
        if (rawReplyDir.exists(entry))
            continue;
        if (!replyDir.remove(entry)) {
            qCCritical(sfdk) << "Failed to remove" << entry << "from" << replyDir;
            return;
        }
    }
}

bool CMakeHelper::doCMakeApiReplyPathMapping(const QDir &src, const QDir &dst, const QString &entry,
        const BuildTargetData &target)
{
    QTC_ASSERT(SdkManager::hasEngine(), return false);
    BuildEngine *const engine = SdkManager::engine();

    FileReader reader;
    if (!reader.fetch(src.filePath(entry))) {
        qCCritical(sfdk).noquote() << reader.errorString();
        return false;
    }

    QString data = QString::fromUtf8(reader.data());

    QTC_CHECK(!engine->sharedSrcPath().toString().contains('\\'));
    data.replace(engine->sharedSrcMountPoint(), engine->sharedSrcPath().toString());

    if (entry.startsWith("cache-")) {
        if (!doCMakeApiCacheReplyPathMapping(&data, target))
            return false;
    } else {
        // Note how this is done selectively for cache-* replies in doCMakeApiCacheReplyPathMapping
        data.replace(QRegularExpression("(/usr/(local/)?include\\b)"),
                target.sysRoot.toString() + "\\1");
        data.replace(QRegularExpression("(/usr/lib\\b)"),
                target.sysRoot.toString() + "\\1");
        data.replace(QRegularExpression("(/usr/share\\b)"),
                target.sysRoot.toString() + "\\1");
    }

    // GCC's system include paths are under the tooling. Map them to the
    // respective directories under the target.
    data.replace(QRegularExpression("/srv/mer/toolings/[^/]+/opt/cross/"),
            target.sysRoot.toString() + "/opt/cross/");

    FileSaver saver(dst.filePath(entry));
    saver.write(data.toUtf8());
    if (!saver.finalize()) {
        qCCritical(sfdk).noquote() << saver.errorString();
        return false;
    }

    return true;
}

bool CMakeHelper::doCMakeApiCacheReplyPathMapping(QString *cacheData, const BuildTargetData &target)
{
    QJsonDocument document = QJsonDocument::fromJson(cacheData->toUtf8());
    QJsonObject root = document.object();

    if (root.value("kind") != "cache") {
        qCCritical(sfdk) << "Not a valid CMake cache file";
        return false;
    }

    QJsonArray entries = root.value("entries").toArray();

    const bool usesQt = Utils::anyOf(entries, [](const QJsonValue &entry) {
        return entry.toObject().value("name") == "Qt5Core_DIR";
    });

    // Map paths and add variables that were filtered out from CMake command line.
    // If not added, QtC would complain about changed configuration.
    updateOrAddToCMakeCacheIf(&entries, "CMAKE_CXX_COMPILER", {"FILEPATH", "STRING"},
            target.toolsPath.toString() + '/' + Constants::WRAPPER_GCC,
            true);
    updateOrAddToCMakeCacheIf(&entries, "CMAKE_C_COMPILER", {"FILEPATH", "STRING"},
            target.toolsPath.toString() + '/' + Constants::WRAPPER_GCC,
            true);
    updateOrAddToCMakeCacheIf(&entries, "CMAKE_COMMAND", {"INTERNAL"},
            target.toolsPath.toString() + '/' + Constants::WRAPPER_CMAKE,
            false);
    updateOrAddToCMakeCacheIf(&entries, "CMAKE_SYSROOT", {"PATH", "STRING"},
            target.sysRoot.toString(),
            true);
    updateOrAddToCMakeCacheIf(&entries, "CMAKE_PREFIX_PATH", {"PATH", "STRING"},
            target.sysRoot.toString() + "/usr",
            true);
    // See qmakeFromCMakeCache() in cmakeprojectimporter.cpp
    updateOrAddToCMakeCacheIf(&entries, "QT_QMAKE_EXECUTABLE", {"FILEPATH", "STRING"},
            target.toolsPath.toString() + '/' + Constants::WRAPPER_QMAKE,
            usesQt);

    for (auto it = entries.begin(); it != entries.end(); ++it) {
        QJsonObject entry = it->toObject();
        const QString name = entry.value("name").toString();
        QString value = entry.value("value").toString();
        if (name != "INCLUDE_INSTALL_DIR") {
            value.replace(QRegularExpression("(/usr/(local/)?include\\b)"),
                        target.sysRoot.toString() + "\\1");
        }
        if (name != "LIB_INSTALL_DIR") {
            value.replace(QRegularExpression("(/usr/lib\\b)"),
                        target.sysRoot.toString() + "\\1");
        }
        if (name != "SHARE_INSTALL_PREFIX") {
            value.replace(QRegularExpression("(/usr/share\\b)"),
                        target.sysRoot.toString() + "\\1");
        }
        entry["value"] = value;
        *it = entry;
    }

    root["entries"] = entries;
    document.setObject(root);

    *cacheData = QString::fromUtf8(document.toJson());

    return true;
}

void CMakeHelper::updateOrAddToCMakeCacheIf(QJsonArray *entries, const QString &name,
        const QStringList &types, const QString &value, bool shouldAdd)
{
    QTC_ASSERT(!types.isEmpty(), return);

    for (auto it = entries->begin(); it != entries->end(); ++it) {
        QJsonObject entry = it->toObject();
        if (entry.value("name") == name) {
            entry["value"] = value;
            *it = entry;
            return;
        }
    }

    if (!shouldAdd)
        return;

    QJsonObject entry;
    entry["name"] = name;
    entry["type"] = types.first();
    entry["value"] = value;

    QJsonObject helpProperty;
    helpProperty["name"] = "HELPSTRING";
    helpProperty["value"] = "Variable specified on the command line";

    QJsonArray properties;
    properties.append(helpProperty);

    entry["properties"] = properties;

    entries->append(entry);
}

} // namespace Sfdk
