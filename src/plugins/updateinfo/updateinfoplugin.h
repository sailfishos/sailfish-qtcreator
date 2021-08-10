/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <extensionsystem/iplugin.h>

QT_BEGIN_NAMESPACE
class QDate;
QT_END_NAMESPACE

namespace UpdateInfo {

namespace Internal {

class UpdateInfoPluginPrivate;

class UpdateInfoPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "UpdateInfo.json")
public:
    enum CheckUpdateInterval {
        DailyCheck,
        WeeklyCheck,
        MonthlyCheck
    };
    Q_ENUM(CheckUpdateInterval)

    UpdateInfoPlugin();
    ~UpdateInfoPlugin() override;

    void extensionsInitialized() override;
    bool initialize(const QStringList &arguments, QString *errorMessage) override;

    bool isAutomaticCheck() const;
    void setAutomaticCheck(bool on);

    CheckUpdateInterval checkUpdateInterval() const;
    void setCheckUpdateInterval(CheckUpdateInterval interval);

    QDate lastCheckDate() const;
    QDate nextCheckDate() const;
    QDate nextCheckDate(CheckUpdateInterval interval) const;

    bool isCheckForUpdatesRunning() const;
    void startCheckForUpdates();

signals:
    void lastCheckDateChanged(const QDate &date);
    void newUpdatesAvailable(bool available);
    void checkForUpdatesRunningChanged(bool running);

private:
    void setLastCheckDate(const QDate &date);

    void startAutoCheckForUpdates();
    void stopAutoCheckForUpdates();
    void doAutoCheckForUpdates();

    void startUpdater();
    void stopCheckForUpdates();

    void collectCheckForUpdatesOutput(const QString &contents);
    void checkForUpdatesFinished();

    void loadSettings() const;
    void saveSettings();

private:
    UpdateInfoPluginPrivate *d;
};

} // namespace Internal
} // namespace UpdateInfo
