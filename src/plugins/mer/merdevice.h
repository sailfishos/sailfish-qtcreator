/****************************************************************************
**
** Copyright (C) 2015-2017 Jolla Ltd.
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

#ifndef MERDEVICE_H
#define MERDEVICE_H

#include <sfdk/device.h>
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

    void fromMap(const QVariantMap &map) override;
    QVariantMap toMap() const override;

    ProjectExplorer::Abi::Architecture architecture() const;
    void setArchitecture(const ProjectExplorer::Abi::Architecture &architecture);
    template<typename T>
    static T architecture_cast(int a);

    bool isCompatibleWith(const ProjectExplorer::Kit *kit) const override;

    static MachineType workaround_machineTypeFromMap(const QVariantMap &map);

    Utils::PortList qmlLivePorts() const;
    QList<Utils::Port> qmlLivePortsList() const;
    void setQmlLivePorts(const Utils::PortList &qmlLivePorts);

protected:
    MerDevice();
    ~MerDevice() override = 0;

private:
    ProjectExplorer::Abi::Architecture m_architecture;
    Utils::PortList m_qmlLivePorts;
};

template<typename T>
T MerDevice::architecture_cast(int a)
{
    static_assert(ProjectExplorer::Abi::ArmArchitecture == static_cast<int>(Sfdk::Device::ArmArchitecture),
            "Abi::Architecture / Device::Architecture mismatch");
    static_assert(ProjectExplorer::Abi::X86Architecture == static_cast<int>(Sfdk::Device::X86Architecture),
            "Abi::Architecture / Device::Architecture mismatch");
    QTC_ASSERT(a >= 0 && a <= Sfdk::Device::X86Architecture, return {});
    return static_cast<T>(a);
}

}

#endif // MERDEVICE_H
