/****************************************************************************
**
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

#include "usersettings_p.h"

#include "sdk_p.h"

#include <utils/filesystemwatcher.h>
#include <utils/fileutils.h>
#include <utils/persistentsettings.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QLockFile>

using namespace Utils;

namespace Sfdk {

namespace {
const char VERSION_KEY[] = "Sfdk.UserSettings.Version";
} // namespace anonymous

class UserSettings::LockFile : public QLockFile
{
public:
    enum LockType {
        ReadLock,
        WriteLock
    };

    LockFile(const FileName &file, LockType type)
        : QLockFile(lockFileName(file, type))
    {
    }

private:
    static QString lockFileName(const FileName &file, LockType type)
    {
        const QString suffix = type == ReadLock
		? QStringLiteral(".rlock")
		: QStringLiteral(".wlock");
        const QString dotFile = file.parentDir().appendPath("." + file.fileName()).toString();
        return dotFile + suffix;
    }
};

/*!
 * \class UserSettings
 * \internal
 */

UserSettings *UserSettings::s_instanceApplyingUpdates = nullptr;

UserSettings::UserSettings(const QString &baseName, const QString &docType, QObject *parent)
    : QObject(parent)
    , m_baseName(baseName)
    , m_docType(docType)
{
}

UserSettings::~UserSettings() = default;

Utils::optional<QVariantMap> UserSettings::load()
{
    QTC_ASSERT(!SdkPrivate::useSystemSettingsOnly(), return {});
    QTC_ASSERT(m_state == NotLoaded, return {});
    m_state = Loaded;

    qCDebug(lib) << "Loading" << m_baseName;

    const FileName fileName = SdkPrivate::isVersionedSettingsEnabled()
        ? sessionScopeFile()
        : userScopeFile();

    int version;
    QVariantMap data;
    std::tie(version, data) = load(fileName);
    if (version > 0) {
        qCDebug(lib) << "Loaded" << m_baseName << "version" << version;
        m_baseVersion = version;
        return data;
    }
    return {};
}

void UserSettings::enableUpdates()
{
    QTC_ASSERT(SdkPrivate::isVersionedSettingsEnabled(), return);
    QTC_ASSERT(m_state == Loaded, return);
    m_state = UpdatesEnabled;

    qCDebug(lib) << "Enabling receiving updates to" << m_baseName;

    const FileName userScopeFile = this->userScopeFile();

    // FileSystemWatcher needs them existing
    const bool mkpathOk = QDir(userScopeFile.parentDir().toString()).mkpath(".");
    QTC_CHECK(mkpathOk);
    if (!userScopeFile.exists()) {
        const bool createFileOk = QFile(userScopeFile.toString()).open(QIODevice::WriteOnly);
        QTC_CHECK(createFileOk);
    }

    m_watcher = std::make_unique<FileSystemWatcher>(this);
    m_watcher->addFile(userScopeFile.toString(), FileSystemWatcher::WatchModifiedDate);
    connect(m_watcher.get(), &FileSystemWatcher::fileChanged,
            this, &UserSettings::checkUpdates);
    checkUpdates();
}

bool UserSettings::save(const QVariantMap &data, QString *errorString)
{
    QTC_ASSERT(!SdkPrivate::useSystemSettingsOnly(), return false);
    QTC_ASSERT(m_state != NotLoaded, return false);

    qCDebug(lib) << "Saving" << m_baseName;

    QString tmpErrorString;
    QString *errorString_ = errorString ? errorString : &tmpErrorString;

    const FileName userScopeFile = this->userScopeFile();

    // QLockFile does not make the path
    const bool mkpathOk = QDir(userScopeFile.parentDir().toString()).mkpath(".");
    QTC_CHECK(mkpathOk);

    LockFile writeLock(userScopeFile, LockFile::WriteLock);
    LockFile readLock(userScopeFile, LockFile::ReadLock);
    if (writeLock.tryLock()) {
        const bool readLockOk = readLock.lock();
        QTC_CHECK(readLockOk);
    }

    bool canSaveUserScopeSettings = false;
    if (writeLock.isLocked() && readLock.isLocked()) {
        // In enableUpdates() we create and empty file to enable FileSystemWatcher operation
        if (userScopeFile.toFileInfo().size() == 0) {
            canSaveUserScopeSettings = true;
        } else {
            int latestVersion;
            std::tie(latestVersion, std::ignore) = load(userScopeFile);
            if (m_baseVersion == latestVersion)
                canSaveUserScopeSettings = true;
        }
    }

    if (canSaveUserScopeSettings)
        ++m_baseVersion;

    if (SdkPrivate::isVersionedSettingsEnabled()) {
        if (!save(sessionScopeFile(), data, errorString_))
            return false;
    } else {
        if (!canSaveUserScopeSettings) {
            // FIXME This should be reported in the UI
            qCWarning(lib) << "Settings changed meanwhile. Discarding own changes.";
            return false;
        }
    }

    if (canSaveUserScopeSettings && !save(userScopeFile, data, errorString_))
        return false;

    return true;
}

bool UserSettings::isApplyingUpdates()
{
    return s_instanceApplyingUpdates != nullptr;
}

void UserSettings::checkUpdates()
{
    qCDebug(lib) << "Checking for updates to" << m_baseName;

    LockFile readLock(userScopeFile(), LockFile::ReadLock);
    const bool readLockOk = readLock.lock();
    QTC_ASSERT(readLockOk, return);

    int version;
    QVariantMap data;
    std::tie(version, data) = load(userScopeFile());
    if (m_baseVersion < version) {
        qCDebug(lib) << "Updates available to" << m_baseName << "version:" << version;
        m_baseVersion = version;
        QTC_CHECK(!s_instanceApplyingUpdates);
        s_instanceApplyingUpdates = this;
        emit updated(data);
        s_instanceApplyingUpdates = nullptr;
    }
}

std::tuple<int, QVariantMap> UserSettings::load(const Utils::FileName &fileName) const
{
    if (!fileName.exists())
        return {};

    PersistentSettingsReader reader;
    if (!reader.load(fileName))
        return {};
    QVariantMap data = reader.restoreValues();
    const int version = data.take(VERSION_KEY).toInt();
    QTC_CHECK(m_baseVersion <= version || version == 0);
#if Q_CC_GNU && Q_CC_GNU < 600
    return std::tuple<int, QVariantMap>{version, data};
#else
    return {version, data};
#endif
}

bool UserSettings::save(const Utils::FileName &fileName, const QVariantMap &data,
        QString *errorString) const
{
    QVariantMap versionedData = data;
    versionedData.insert(VERSION_KEY, m_baseVersion);
    PersistentSettingsWriter writer(fileName, m_docType);
    return writer.save(versionedData, errorString);
}

Utils::FileName UserSettings::sessionScopeFile() const
{
    return SdkPrivate::settingsFile(SdkPrivate::SessionScope, m_baseName);
}

Utils::FileName UserSettings::userScopeFile() const
{
    return SdkPrivate::settingsFile(SdkPrivate::UserScope, m_baseName);
}

} // namespace Sfdk
