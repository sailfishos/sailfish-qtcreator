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

#include "emulator.h"

#include <utils/portlist.h>

namespace QSsh {
class SshConnectionParameters;
}

namespace Utils {
class FileSystemWatcher;
}

namespace Sfdk {

class VirtualMachineInfo;

class EmulatorPrivate
{
    Q_DECLARE_PUBLIC(Emulator)

public:
    explicit EmulatorPrivate(Emulator *q);
    ~EmulatorPrivate();

    static EmulatorPrivate *get(Emulator *q) { return q->d_func(); }

    QString mac() const { return mac_; }
    QString subnet() const { return subnet_; }

    QVariantMap toMap() const;
    bool fromMap(const QVariantMap &data);

    bool initVirtualMachine(const QUrl &vmUri);
    void enableUpdates();
    void updateOnce();
    void updateVmProperties(const QObject *context, const Functor<bool> &functor);

private:
    void setSharedConfigPath(const Utils::FileName &sharedConfigPath);
    void setSharedSshPath(const Utils::FileName &sharedSshPath);
    void setSshParameters(const QSsh::SshConnectionParameters &sshParameters);
    void setFreePorts(const Utils::PortList &freePorts);
    void setQmlLivePorts(const Utils::PortList &qmlLivePorts);
    void setDisplayProperties(const DeviceModelData &deviceModel, Qt::Orientation orientation,
            bool viewScaled);

    static bool updateCompositorConfig(const QString &file, const QSize &displaySize,
            const QSize &displayResolution, bool viewScaled);
    static bool updateDconfDb(const QString &file, const QString &dconf);

private:
    Emulator *const q_ptr;
    std::unique_ptr<VirtualMachine> virtualMachine;
    bool autodetected = false;
    Utils::FileName sharedConfigPath;
    Utils::FileName sharedSshPath;
    Utils::PortList freePorts;
    Utils::PortList qmlLivePorts;
    QString factorySnapshot;
    QString mac_;
    QString subnet_;
    DeviceModelData deviceModel;
    Qt::Orientation orientation = Qt::Vertical;
    bool viewScaled = false;
};

class EmulatorManager : public QObject
{
    Q_OBJECT

public:
    explicit EmulatorManager(QObject *parent);
    ~EmulatorManager() override;
    static EmulatorManager *instance();

    static QString installDir();

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

    static QVariantMap toMap(const DeviceModelData &deviceModel);
    static void fromMap(DeviceModelData *deviceModel, const QVariantMap &data);

signals:
    void emulatorAdded(int index);
    void aboutToRemoveEmulator(int index);
    void deviceModelsChanged();

private:
    QVariantMap toMap() const;
    void fromMap(const QVariantMap &data, bool merge = false);
    void enableUpdates();
    void updateOnce();
    void checkSystemSettings();
    void saveSettings(QStringList *errorStrings) const;
    void fixDeviceModelsInUse(const QObject *context, const Functor<bool> &functor);
    static Utils::FileName systemSettingsFile();

private:
    static EmulatorManager *s_instance;
    const std::unique_ptr<UserSettings> m_userSettings;
    int m_version = 0;
    QString m_installDir;
    std::vector<std::unique_ptr<Emulator>> m_emulators;
    QMap<QString, DeviceModelData> m_deviceModels;
};

} // namespace Sfdk
