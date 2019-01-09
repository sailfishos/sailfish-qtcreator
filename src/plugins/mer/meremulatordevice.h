#ifndef MEREMULATORDEVICE_H
#define MEREMULATORDEVICE_H

/****************************************************************************
**
** Copyright (C) 2012 - 2018 Jolla Ltd.
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
#include <QPointer>
#include <QSharedPointer>
#include <QSize>

QT_BEGIN_NAMESPACE
class QTimer;
QT_END_NAMESPACE

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
    QString dconf() const { return d->dconf; }

    void fromMap(const QVariantMap &map);
    QVariantMap toMap() const;

private:
    struct Data : QSharedData
    {
        QString name;
        QSize displayResolution;
        QSize displaySize;
        QString dconf;
    };
    QSharedDataPointer<Data> d;
};

class MerEmulatorDevice : public MerDevice
{
    Q_DECLARE_TR_FUNCTIONS(Mer::Internal::MerEmulatorDevice)

public:
    typedef QSharedPointer<MerEmulatorDevice> Ptr;
    typedef QSharedPointer<const MerEmulatorDevice> ConstPtr;

    static Ptr create(Core::Id id = Core::Id());
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

    QString deviceModel() const;
    void setDeviceModel(const QString &deviceModel);
    Qt::Orientation orientation() const;
    void setOrientation(Qt::Orientation orientation);
    bool isViewScaled() const;
    void setViewScaled(bool viewScaled);

    MerConnection *connection() const;

private:
    MerEmulatorDevice(Core::Id id);
    MerEmulatorDevice(const MerEmulatorDevice &other);

    friend class MerEmulatorDeviceManager;
    void updateConnection() const;

    void scheduleSetVideoMode();
    void setVideoMode();
    void updateDconfDb();

private:
    QSharedPointer<MerConnection> m_connection; // all clones share the connection
    QString m_mac;
    QString m_subnet;
    QString m_sharedConfigPath;
    QString m_deviceModel;
    Qt::Orientation m_orientation;
    bool m_viewScaled;
    QPointer<QTimer> m_setVideoModeTimer;
};

class MerEmulatorDeviceManager : public QObject
{
    Q_OBJECT

public:
    MerEmulatorDeviceManager(QObject *parent = 0);
    static MerEmulatorDeviceManager *instance();
    ~MerEmulatorDeviceManager() override;

    static bool isStored(const MerEmulatorDevice::ConstPtr &device);
    static bool restorePorts(const MerEmulatorDevice::Ptr &device);

signals:
    void storedDevicesChanged();

private slots:
    void onDeviceCreated(const ProjectExplorer::IDevice::Ptr &device);
    void onDeviceAdded(Core::Id id);
    void onDeviceRemoved(Core::Id id);
    void onDeviceListReplaced();

private:
    static MerEmulatorDeviceManager *s_instance;
    QHash<Core::Id, quint16> m_deviceSshPortCache;
    QHash<Core::Id, Utils::PortList> m_deviceQmlLivePortsCache;
};

}
}

#endif // MEREMULATORDEVICE_H
