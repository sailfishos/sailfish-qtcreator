/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
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

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/devicesupport/idevicefactory.h>

#include <utils/qtcprocess.h>

namespace WinRt {
namespace Internal {

class WinRtDevice final : public ProjectExplorer::IDevice
{
public:
    typedef QSharedPointer<WinRtDevice> Ptr;
    typedef QSharedPointer<const WinRtDevice> ConstPtr;

    static Ptr create() { return Ptr(new WinRtDevice); }

    ProjectExplorer::IDeviceWidget *createWidget() override;
    ProjectExplorer::DeviceProcessSignalOperation::Ptr signalOperation() const override;
    void fromMap(const QVariantMap &map) override;
    QVariantMap toMap() const override;

    static QString displayNameForType(Core::Id type);
    int deviceId() const { return m_deviceId; }
    void setDeviceId(int deviceId) { m_deviceId = deviceId; }

private:
    WinRtDevice();

    int m_deviceId = -1;
};

class WinRtDeviceFactory final : public QObject, public ProjectExplorer::IDeviceFactory
{
    Q_OBJECT
public:
    explicit WinRtDeviceFactory(Core::Id deviceType);

    void autoDetect();
    void onPrerequisitesLoaded();

private:
    void onProcessError();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

    static bool allPrerequisitesLoaded();
    QString findRunnerFilePath() const;
    void parseRunnerOutput(const QByteArray &output) const;

    Utils::QtcProcess *m_process = nullptr;
    bool m_initialized = false;
};

} // Internal
} // WinRt
