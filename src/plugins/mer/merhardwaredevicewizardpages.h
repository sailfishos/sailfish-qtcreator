/****************************************************************************
**
** Copyright (C) 2012-2015 Jolla Ltd.
** Copyright (C) 2019-2020 Open Mobile Platform LLC.
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

#ifndef MERHARDWAREDEVICEWIZARDPAGES_H
#define MERHARDWAREDEVICEWIZARDPAGES_H

#include "merhardwaredevice.h"

#include <projectexplorer/abi.h>

#include <QWizardPage>

namespace QSsh {
    class SshConnectionParameters;
}

namespace Sfdk {
    class BuildEngine;
}

namespace Mer {
namespace Internal {

namespace Ui {
    class MerHardwareDeviceWizardSelectionPage;
    class MerHardwareDeviceWizardConnectionTestPage;
    class MerHardwareDeviceWizardSetupPage;
}

class MerHardwareDeviceWizardSelectionPage : public QWizardPage
{
    Q_OBJECT
public:
    explicit MerHardwareDeviceWizardSelectionPage(QWidget *parent = 0);

    void setDevice(const MerHardwareDevice::Ptr &device);

    QString hostName() const;
    QString userName() const;
    int timeout() const;
    int sshPort() const;

    bool isComplete() const override;
    bool validatePage() override;

private:
    Ui::MerHardwareDeviceWizardSelectionPage *m_ui;
    MerHardwareDevice::Ptr m_device;
};

class MerHardwareDeviceWizardConnectionTestPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit MerHardwareDeviceWizardConnectionTestPage(QWidget *parent = 0);

    void setDevice(const MerHardwareDevice::Ptr &device);

    void initializePage() override;
    bool isComplete() const override;
    bool validatePage() override;

private slots:
    void testConnection();

private:
    static ProjectExplorer::Abi detectAbi(
            const QSsh::SshConnectionParameters &sshParams, bool *ok, QString *errorMessage);
    static QString detectDeviceName(const QSsh::SshConnectionParameters &sshParams, bool *ok, QString *errorMessage);
    static bool detectSdkClientTools(const QSsh::SshConnectionParameters &sshParams, bool *ok, QString *errorMessage);
    static bool askInstallSdkClientTools(QString *password);
    static bool installSdkClientTools(const QSsh::SshConnectionParameters &sshParams, const QString &password, QString *errorMessage);
    void showErrorMessageDialog(const QString &error, const QString &message);

private:
    Ui::MerHardwareDeviceWizardConnectionTestPage *m_ui;
    MerHardwareDevice::Ptr m_device;
    ProjectExplorer::Abi m_abi;
    QString m_deviceName;
    bool m_sdkClientToolsInstalled;
    bool m_isIdle;
    bool m_connectionTestOk;
};

class MerHardwareDeviceWizardSetupPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit MerHardwareDeviceWizardSetupPage(QWidget *parent = 0);

    void setDevice(const MerHardwareDevice::Ptr &device);

    void initializePage() override;

    QString configName() const;
    QString freePorts() const;

    bool isComplete() const override;
    bool validatePage() override;

private:
    Ui::MerHardwareDeviceWizardSetupPage *m_ui;
    MerHardwareDevice::Ptr m_device;
};

}
}

#endif // MERHARDWAREDEVICEPAGES_H
