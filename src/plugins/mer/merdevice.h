/****************************************************************************
**
** Copyright (C) 2015 Jolla Ltd.
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

#ifndef MERDEVICE_H
#define MERDEVICE_H

#include <projectexplorer/abi.h>
#include <remotelinux/linuxdevice.h>
#include <utils/portlist.h>

#include <QSharedPointer>

namespace Mer {

class MerDevice : public RemoteLinux::LinuxDevice
{
    Q_DECLARE_TR_FUNCTIONS(Mer::Internal::MerDevice)

public:
    typedef QSharedPointer<MerDevice> Ptr;
    typedef QSharedPointer<const MerDevice> ConstPtr;

    QString displayType() const override;

    void fromMap(const QVariantMap &map) override;
    QVariantMap toMap() const override;

    virtual ProjectExplorer::Abi::Architecture architecture() const = 0;

    static MachineType workaround_machineTypeFromMap(const QVariantMap &map);

    void setSharedSshPath(const QString &sshPath);
    QString sharedSshPath() const;

    Utils::PortList qmlLivePorts() const;
    void setQmlLivePorts(const Utils::PortList &qmlLivePorts);
    QList<Utils::Port> qmlLivePortsList() const;

protected:
    MerDevice();
    MerDevice(const QString &name, MachineType machineType, Origin origin, Core::Id id);
    ~MerDevice() override = 0;

private:
    MerDevice &operator=(const MerDevice &);

private:
    QString m_sharedSshPath;
    Utils::PortList m_qmlLivePorts;
};

}

#endif // MERDEVICE_H
