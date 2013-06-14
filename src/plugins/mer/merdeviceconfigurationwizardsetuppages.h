/****************************************************************************
**
** Copyright (C) 2012 - 2013 Jolla Ltd.
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

#ifndef MERDEVICECONFIGURATIONWIZARDSETUPPAGES_H
#define MERDEVICECONFIGURATIONWIZARDSETUPPAGES_H

#include <remotelinux/genericlinuxdeviceconfigurationwizardpages.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <ssh/sshconnection.h>

#include <QWizardPage>

namespace Mer {
namespace Internal {

namespace Ui {
class MerDeviceConfigWizardDeviceTypePage;
class MerDeviceConfigWizardGeneralPage;
class MerDeviceConfigWizardStartPage;
class MerDeviceConfigWizardCheckPreviousKeySetupPage;
class MerDeviceConfigWizardReuseKeysCheckPage;
class MerDeviceConfigWizardKeyCreationPage;
}

struct WizardData
{
    QString configName;
    QString hostName;
    QSsh::SshConnectionParameters::AuthenticationType authType;
    ProjectExplorer::IDevice::MachineType machineType;
    Core::Id deviceType;
    QString privateKeyFilePath;
    QString publicKeyFilePath;
    QString userName;
    QString password;
    QString freePorts;
    int sshPort;
    int timeout;
};

class MerDeviceConfigWizardGeneralPage : public QWizardPage
{
    Q_OBJECT
public:
    explicit MerDeviceConfigWizardGeneralPage(const WizardData &wizardData, QWidget *parent = 0);

    virtual void initializePage();
    virtual bool isComplete() const;
    QString configName() const;
    QString hostName() const;
    QString userName() const;
    QString password() const;
    QString freePorts() const;
    QSsh::SshConnectionParameters::AuthenticationType authType() const;
    ProjectExplorer::IDevice::MachineType machineType() const;
    int timeout() const;
    int sshPort() const;

private slots:
    void authTypeChanged();
    void machineTypeChanged();
    void onVirtualMachineChanged(const QString &vmName);

private:
    Ui::MerDeviceConfigWizardGeneralPage *m_ui;
    const WizardData &m_wizardData;
};

class MerDeviceConfigWizardFinalPage :
        public RemoteLinux::GenericLinuxDeviceConfigurationWizardFinalPage
{
    Q_OBJECT
public:
    MerDeviceConfigWizardFinalPage(const WizardData &wizardData, QWidget *parent);

private slots:
    void startEmulator();

private:
    QString infoText() const;

private:
    const WizardData &m_wizardData;
};

} // Mer
} // Internal

#endif // MERDEVICECONFIGURATIONWIZARDSETUPPAGES_H
