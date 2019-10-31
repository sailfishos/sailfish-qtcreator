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

#include "sdk.h"

namespace Utils {
class FileName;
}

namespace Sfdk {

class BuildEngineManager;
class CommandQueue;
class EmulatorManager;
class DeviceManager;
class VirtualMachineFactory;

class SdkPrivate : public QObject
{
    Q_OBJECT
    friend class Sdk;

public:
    enum SettingsScope {
        SystemScope,
        UserScope,
        SessionScope,
    };
    Q_ENUM(SettingsScope)

    SdkPrivate(Sdk *q);
    ~SdkPrivate();

    static SdkPrivate *instance() { return Sdk::s_instance->d_func(); }

    static bool isVersionedSettingsEnabled() { return instance()->options_ & Sdk::VersionedSettings; }
    static bool isUpdatesEnabled() { return instance()->updatesEnabled; }
    static bool useSystemSettingsOnly() { return instance()->options_ & Sdk::SystemSettingsOnly; }
    static bool useCachedVmInfo() { return instance()->options_ & Sdk::CachedVmInfo; }

    static QDateTime lastMaintained();

    static CommandQueue *commandQueue() { return instance()->commandQueue_.get(); }

    static Utils::FileName libexecPath();
    static Utils::FileName settingsFile(SettingsScope scope, const QString &basename);
    static Utils::FileName settingsLocation(SettingsScope scope);
    static Utils::FileName cacheFile(const QString &basename);
    static Utils::FileName cacheLocation();

signals:
    void enableUpdatesRequested();
    void updateOnceRequested();
    void saveSettingsRequested(QStringList *errorStrings);

private:
    Sdk::Options options_;
    bool updatesEnabled = false;
    std::unique_ptr<CommandQueue> commandQueue_;
    std::unique_ptr<VirtualMachineFactory> virtualMachineFactory;
    std::unique_ptr<BuildEngineManager> buildEngineManager;
    std::unique_ptr<EmulatorManager> emulatorManager;
    std::unique_ptr<DeviceManager> deviceManager;
};

} // namespace Sfdk
