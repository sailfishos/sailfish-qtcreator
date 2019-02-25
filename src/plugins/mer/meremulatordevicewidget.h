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

#ifndef MEREMULATORDEVICEWIDGET_H
#define MEREMULATORDEVICEWIDGET_H

#include <projectexplorer/devicesupport/idevicewidget.h>

namespace Mer {
namespace Internal {

namespace Ui {
class MerEmulatorDeviceWidget;
}

class MerVirtualMachineSettingsWidget;

class MerEmulatorDeviceWidget : public ProjectExplorer::IDeviceWidget
{
    Q_OBJECT
public:
    explicit MerEmulatorDeviceWidget(const ProjectExplorer::IDevice::Ptr &deviceConfig,
                                          QWidget *parent = 0);
    ~MerEmulatorDeviceWidget() override;

private slots:
    void onStoredDevicesChanged();
    void onVirtualMachineOffChanged(bool vmOff);
    void timeoutEditingFinished();
    void userNameEditingFinished();
    void handleSshPortChanged();
    void handleFreePortsChanged();
    void handleQmlLivePortsChanged();
    void handleMemorySizeMbChanged(int sizeMb);
    void handleCpuCountChanged(int cpuCount);
    void handleVdiCapacityMbChanged(int sizeMb);

private:
    void updateDeviceFromUi();
    void updatePortInputsEnabledState();
    void updatePortsWarningLabel();
    void updateQmlLivePortsWarningLabel();
    void updateSystemParameters();
    void initGui();

    Ui::MerEmulatorDeviceWidget *m_ui;
    MerVirtualMachineSettingsWidget *m_virtualMachineSettingsWidget;
};

} // Internal
} // Mer

#endif // MEREMULATORDEVICEWIDGET_H
