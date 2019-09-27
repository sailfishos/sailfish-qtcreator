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

#include "sfdkglobal.h"

#include "asynchronous.h"

#include <QObject>

namespace Sfdk {

class BuildEngine;
class Device;
class DeviceModelData;
class Emulator;
class VirtualMachineDescriptor;

class SdkPrivate;
class SFDK_EXPORT Sdk : public QObject
{
    Q_OBJECT

public:
    enum Option {
        NoOption = 0x0,
        VersionedSettings = 0x1,
        SystemSettingsOnly = 0x2,
    };
    Q_DECLARE_FLAGS(Options, Option)
    Q_FLAG(Options)

    Sdk(Options options = NoOption);
    ~Sdk() override;
    static Sdk *instance();

    static void enableUpdates();
    static bool saveSettings(QStringList *errorStrings);

    static QString installationPath();

    static void unusedVirtualMachines(const QObject *context,
            const Functor<const QList<VirtualMachineDescriptor> &, bool> &functor);

    static QList<BuildEngine *> buildEngines();
    static BuildEngine *buildEngine(const QUrl &uri);
    static void createBuildEngine(const QUrl &virtualMachineUri, const QObject *context,
            const Functor<std::unique_ptr<BuildEngine> &&> &functor);
    static int addBuildEngine(std::unique_ptr<BuildEngine> &&buildEngine);
    static void removeBuildEngine(const QUrl &uri);

    static QList<Emulator *> emulators();
    static Emulator *emulator(const QUrl &uri);
    static void createEmulator(const QUrl &virtualMachineUri, const QObject *context,
        const Functor<std::unique_ptr<Emulator> &&> &functor);
    static int addEmulator(std::unique_ptr<Emulator> &&emulator);
    static void removeEmulator(const QUrl &uri);

    static QList<DeviceModelData> deviceModels();
    static DeviceModelData deviceModel(const QString &name);
    static void setDeviceModels(const QList<DeviceModelData> &deviceModels,
            const QObject *context, const Functor<bool> &functor);

    static QList<Device *> devices();
    static Device *device(const QString &id);
    static int addDevice(std::unique_ptr<Device> &&device);
    static void removeDevice(const QString &id);

signals:
    void buildEngineAdded(int index);
    void aboutToRemoveBuildEngine(int index);
    void emulatorAdded(int index);
    void aboutToRemoveEmulator(int index);
    void deviceModelsChanged();
    void deviceAdded(int index);
    void aboutToRemoveDevice(int index);

private:
    static Sdk *s_instance;
    std::unique_ptr<SdkPrivate> d_ptr;
    Q_DISABLE_COPY(Sdk)
    Q_DECLARE_PRIVATE(Sdk)
};

Q_DECLARE_OPERATORS_FOR_FLAGS(Sdk::Options);

} // namespace Sfdk
