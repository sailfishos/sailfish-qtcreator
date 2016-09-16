#ifndef MEREMULATORDEVICE_H
#define MEREMULATORDEVICE_H

/****************************************************************************
**
** Copyright (C) 2012 - 2014 Jolla Ltd.
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

#include <QCoreApplication>
#include <QSharedPointer>
#include <QSize>

namespace Mer {
namespace Internal {

class MerConnection;

class MerEmulatorDeviceModel
{
public:
    MerEmulatorDeviceModel()
        : d(new Data)
    {}

    bool isNull() const { return d->name.isNull() || d->displayResolution.isNull() || d->displaySize.isNull(); }

    QString name() const { return d->name; }
    QSize displayResolution() const { return d->displayResolution; }
    QSize displaySize() const { return d->displaySize; }

    void fromMap(const QVariantMap &map);
    QVariantMap toMap() const;

private:
    struct Data : QSharedData
    {
        QString name;
        QSize displayResolution;
        QSize displaySize;
    };
    QSharedDataPointer<Data> d;
};

class MerEmulatorDevice : public MerDevice
{
    Q_DECLARE_TR_FUNCTIONS(Mer::Internal::MerEmulatorDevice)

public:
    typedef QSharedPointer<MerEmulatorDevice> Ptr;
    typedef QSharedPointer<const MerEmulatorDevice> ConstPtr;

    static Ptr create();
    ProjectExplorer::IDevice::Ptr clone() const;
    ~MerEmulatorDevice() override;

    ProjectExplorer::IDeviceWidget *createWidget() override;
    QList<Core::Id> actionIds() const override;
    QString displayNameForActionId(Core::Id actionId) const override;
    void executeAction(Core::Id actionId, QWidget *parent) override;

    ProjectExplorer::DeviceTester *createDeviceTester() const override;

    void fromMap(const QVariantMap &map) override;
    QVariantMap toMap() const override;

    ProjectExplorer::Abi::Architecture architecture() const override;

    void setSharedConfigPath(const QString &configPath);
    QString sharedConfigPath() const;

    void setVirtualMachine(const QString& machineName);
    QString virtualMachine() const;

    void setMac(const QString& mac);
    QString mac() const;

    void setSubnet(const QString& subnet);
    QString subnet() const;

    void generateSshKey(const QString& user) const;

    QSsh::SshConnectionParameters sshParametersForUser(const QSsh::SshConnectionParameters &sshParams, const QLatin1String &user) const;

    QMap<QString, MerEmulatorDeviceModel> availableDeviceModels() const;
    QString deviceModel() const;
    void setDeviceModel(const QString &deviceModel);
    Qt::Orientation orientation() const;
    void setOrientation(Qt::Orientation orientation);
    bool isViewScaled() const;
    void setViewScaled(bool viewScaled);

    MerConnection *connection() const;
    // ATTENTION! Call this when sshParameters are changed! Unfortunately
    // IDevice API does not allow to hook this.
    void updateConnection();

private:
    MerEmulatorDevice();
    MerEmulatorDevice(const MerEmulatorDevice &other);

    void updateAvailableDeviceModels();
    void setVideoMode();
    void updateDconfDb(const QVariantMap &fullDeviceModelData);
    QVariantMap readFullDeviceModelData() const;

private:
    QSharedPointer<MerConnection> m_connection; // all clones share the connection
#if __cplusplus >= 201103L
    QMetaObject::Connection m_virtualMachineChangedConnection;
#endif
    QString m_mac;
    QString m_subnet;
    QString m_sharedConfigPath;
    QString m_deviceModel;
    QMap<QString, MerEmulatorDeviceModel> m_availableDeviceModels;
    Qt::Orientation m_orientation;
    bool m_viewScaled;
};

}
}

#endif // MEREMULATORDEVICE_H
