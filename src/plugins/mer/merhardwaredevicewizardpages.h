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

#ifndef MERHARDWAREDEVICEWIZARDPAGES_H
#define MERHARDWAREDEVICEWIZARDPAGES_H

#include <projectexplorer/abi.h>

#include <QWizardPage>

namespace QSsh {
    class SshConnectionParameters;
}

namespace Mer {
namespace Internal {

namespace Ui {
    class MerHardwareDeviceWizardSelectionPage;
    class MerHardwareDeviceWizardSetupPage;
}

class MerHardwareDeviceWizardSelectionPage : public QWizardPage
{
    Q_OBJECT
public:
    explicit MerHardwareDeviceWizardSelectionPage(QWidget *parent = 0);

    QString hostName() const;
    QString userName() const;
    int timeout() const;
    int sshPort() const;

    ProjectExplorer::Abi::Architecture architecture() const;
    QString deviceName() const;

    bool isComplete() const override;

private slots:
    void handleTestConnectionClicked();

private:
    static ProjectExplorer::Abi::Architecture detectArchitecture(
            const QSsh::SshConnectionParameters &sshParams, bool *ok);
    static QString detectDeviceName(const QSsh::SshConnectionParameters &sshParams, bool *ok);

private:
    Ui::MerHardwareDeviceWizardSelectionPage *m_ui;
    ProjectExplorer::Abi::Architecture m_architecture;
    QString m_deviceName;
    bool m_isIdle;
    bool m_connectionTestOk;
};

class MerHardwareDeviceWizardSetupPage : public QWizardPage
{
    Q_OBJECT
public:
    explicit MerHardwareDeviceWizardSetupPage(QWidget *parent = 0);

    void initializePage() override;

    QString configName() const;
    QString freePorts() const;
    QString publicKeyFilePath() const;
    QString privateKeyFilePath() const;
    bool isNewSshKeysRquired() const;
    QString sharedSshPath() const;

    bool isComplete() const override;

private slots:
    void handleSdkVmChanged(const QString &vmName);

private:
    Ui::MerHardwareDeviceWizardSetupPage *m_ui;
};

}
}

#endif // MERHARDWAREDEVICEPAGES_H
