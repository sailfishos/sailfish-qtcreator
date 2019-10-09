#ifndef MEREMULATORDEVICE_H
#define MEREMULATORDEVICE_H

/****************************************************************************
**
** Copyright (C) 2012-2019 Jolla Ltd.
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

#include "merdevice.h"

#include <QSharedPointer>
#include <QPointer>

namespace Sfdk {
class Emulator;
}

namespace Mer {

class Q_DECL_EXPORT MerEmulatorDevice : public MerDevice
{
    Q_DECLARE_TR_FUNCTIONS(Mer::MerEmulatorDevice)

public:
    typedef QSharedPointer<MerEmulatorDevice> Ptr;
    typedef QSharedPointer<const MerEmulatorDevice> ConstPtr;

    static Ptr create() { return Ptr(new MerEmulatorDevice); }
    ProjectExplorer::IDevice::Ptr clone() const;
    ~MerEmulatorDevice() override;

    ProjectExplorer::IDeviceWidget *createWidget() override;
    ProjectExplorer::DeviceTester *createDeviceTester() const override;

    void fromMap(const QVariantMap &map) override;
    QVariantMap toMap() const override;

    ProjectExplorer::Abi::Architecture architecture() const override;

    Utils::PortList qmlLivePorts() const override;

    Sfdk::Emulator *emulator() const;
    void setEmulator(Sfdk::Emulator *emulator);

    static void generateSshKey(Sfdk::Emulator *emulator, const QString& user);

    static Core::Id idFor(const Sfdk::Emulator &emulator);
    static QString privateKeyFile(Core::Id emulatorId, const QString &user);
    static void doFactoryReset(Sfdk::Emulator *emulator, QWidget *parent);

private:
    MerEmulatorDevice();
    MerEmulatorDevice(const MerEmulatorDevice &other);
    void init();

private:
    QPointer<Sfdk::Emulator> m_emulator;
};

class MerEmulatorDeviceManager : public QObject
{
    Q_OBJECT

public:
    MerEmulatorDeviceManager(QObject *parent = 0);
    static MerEmulatorDeviceManager *instance();
    ~MerEmulatorDeviceManager() override;

private:
    void onEmulatorAdded(int index);
    void onAboutToRemoveEmulator(int index);
    void startWatching(Sfdk::Emulator *emulator);
    void stopWatching(Sfdk::Emulator *emulator);

private:
    static MerEmulatorDeviceManager *s_instance;
};

}

#endif // MEREMULATORDEVICE_H
