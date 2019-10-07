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

#pragma once

#include <utils/optional.h>

#include <QObject>

#include <memory>

namespace Utils {
class FileName;
class FileSystemWatcher;
}

namespace Sfdk {

class UserSettings : public QObject
{
    Q_OBJECT

    class LockFile;

    enum State {
        NotLoaded,
        Loaded,
        UpdatesEnabled
    };

public:
    explicit UserSettings(const QString &baseName, const QString &docType, QObject *parent);
    ~UserSettings() override;

    Utils::optional<QVariantMap> load();
    void enableUpdates();
    bool save(const QVariantMap &data, QString *errorString);

    static bool isApplyingUpdates();

signals:
    void updated(const QVariantMap &data);

private:
    void checkUpdates();
    std::tuple<int, QVariantMap> load(const Utils::FileName &fileName) const;
    bool save(const Utils::FileName &fileName, const QVariantMap &data,
            QString *errorString) const;
    Utils::FileName sessionScopeFile() const;
    Utils::FileName userScopeFile() const;

private:
    static UserSettings *s_instanceApplyingUpdates;
    State m_state = NotLoaded;
    const QString m_baseName;
    const QString m_docType;
    std::unique_ptr<Utils::FileSystemWatcher> m_watcher;
    int m_baseVersion = 0;
};

} // namespace Sfdk
