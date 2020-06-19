/****************************************************************************
**
** Copyright (C) 2016 BlackBerry Limited. All rights reserved.
** Contact: KDAB (info@kdab.com)
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

#include "qnx_export.h"

#include <remotelinux/linuxdevice.h>

namespace Qnx {
namespace Internal {

class QnxDevice final : public RemoteLinux::LinuxDevice
{
    Q_DECLARE_TR_FUNCTIONS(Qnx::Internal::QnxDevice)

public:
    using Ptr = QSharedPointer<QnxDevice>;
    using ConstPtr = QSharedPointer<const QnxDevice>;

    static Ptr create() { return Ptr(new QnxDevice); }

    ProjectExplorer::PortsGatheringMethod::Ptr portsGatheringMethod() const override;
    ProjectExplorer::DeviceProcessList *createProcessListModel(QObject *parent) const override;
    ProjectExplorer::DeviceProcessSignalOperation::Ptr signalOperation() const override;

    ProjectExplorer::DeviceTester *createDeviceTester() const override;
    ProjectExplorer::DeviceProcess *createProcess(QObject *parent) const override;

    int qnxVersion() const;

    void fromMap(const QVariantMap &map) override;
    QVariantMap toMap() const override;

protected:
    QString interruptProcessByNameCommandLine(const QString &filePath) const;
    QString killProcessByNameCommandLine(const QString &filePath) const;

private:
    QnxDevice();

    void updateVersionNumber() const;

    mutable int m_versionNumber = 0;
};

class QnxDeviceFactory final : public ProjectExplorer::IDeviceFactory
{
public:
    QnxDeviceFactory();

    ProjectExplorer::IDevice::Ptr create() const override;
};

} // namespace Internal
} // namespace Qnx
