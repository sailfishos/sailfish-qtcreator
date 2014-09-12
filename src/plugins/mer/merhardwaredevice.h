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

#ifndef MERDEVICE_H
#define MERDEVICE_H

#include <remotelinux/linuxdevice.h>

namespace Mer {
namespace Internal {

class MerHardwareDevice : public RemoteLinux::LinuxDevice
{
    Q_DECLARE_TR_FUNCTIONS(Mer::Internal::MerDevice)
public:
    typedef QSharedPointer<MerHardwareDevice> Ptr;
    typedef QSharedPointer<const MerHardwareDevice> ConstPtr;

    static Ptr create();
    static Ptr create(const QString &name, Origin origin = ManuallyAdded, Core::Id id = Core::Id());

    QString displayType() const;
    ProjectExplorer::IDevice::Ptr clone() const;

    void setSharedSshPath(const QString &sshPath);
    QString sharedSshPath() const;

    void fromMap(const QVariantMap &map);
    QVariantMap toMap() const;

    QList<Core::Id> actionIds() const;
    QString displayNameForActionId(Core::Id actionId) const;
    void executeAction(Core::Id actionId, QWidget *parent);
    ProjectExplorer::IDeviceWidget* createWidget();

protected:
    MerHardwareDevice();
    MerHardwareDevice(const QString &name, Core::Id type, MachineType machineType, Origin origin,
              Core::Id id);
private:
    MerHardwareDevice &operator=(const MerHardwareDevice &);
private:
    QString m_sharedSshPath;
};

}
}

#endif // MERDEVICE_H
