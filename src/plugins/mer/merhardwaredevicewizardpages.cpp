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
#include "merconstants.h"
#include "mersdkmanager.h"
#include "mervirtualboxmanager.h"
#include "merhardwaredevicewizardpages.h"
#include "merhardwaredevicewizard.h"
#include "merconnectionmanager.h"
#include "ui_merhardwaredevicewizardgeneralpage.h"
#include "ui_merhardwaredevicewizardkeypage.h"
#include <projectexplorer/devicesupport/devicemanager.h>
#include <utils/qtcassert.h>
#include <utils/portlist.h>
#include <QDir>

namespace Mer {
namespace Internal {


MerHardwareDeviceWizardGeneralPage::MerHardwareDeviceWizardGeneralPage(QWidget *parent)
    : QWizardPage(parent)
    , m_ui(new Ui::MerHardwareDeviceWizardGeneralPage)
{
    m_ui->setupUi(this);
    setTitle(tr("General Information"));

    QString preferredName = tr("SailfishOS Device");

    int i = 1;
    QString tryName = preferredName;
    while (ProjectExplorer::DeviceManager::instance()->hasDevice(tryName))
        tryName = preferredName + QString::number(++i);

    m_ui->configLineEdit->setText(tryName);
    m_ui->hostNameLineEdit->setText(QLatin1String("192.168.2.15"));

    m_ui->usernameLineEdit->setText(QLatin1String(Constants::MER_DEVICE_DEFAULTUSER));
    m_ui->sshPortSpinBox->setMinimum(1);
    m_ui->sshPortSpinBox->setMaximum(65535);
    m_ui->sshPortSpinBox->setValue(22);

    m_ui->timeoutSpinBox->setMinimum(1);
    m_ui->timeoutSpinBox->setMaximum(65535);
    m_ui->timeoutSpinBox->setValue(10);

    m_ui->freePortsLineEdit->setText(QLatin1String("10000-10100"));

    connect(m_ui->configLineEdit, SIGNAL(textChanged(QString)), SIGNAL(completeChanged()));
    connect(m_ui->hostNameLineEdit, SIGNAL(textChanged(QString)), SIGNAL(completeChanged()));
}

bool MerHardwareDeviceWizardGeneralPage::isComplete() const
{
    return !configName().isEmpty()
            && !hostName().isEmpty()
            && !userName().isEmpty()
            && !ProjectExplorer::DeviceManager::instance()->hasDevice(configName());
}

QString MerHardwareDeviceWizardGeneralPage::configName() const
{
    return m_ui->configLineEdit->text().trimmed();
}

QString MerHardwareDeviceWizardGeneralPage::hostName() const
{
    return m_ui->hostNameLineEdit->text().trimmed();
}

QString MerHardwareDeviceWizardGeneralPage::userName() const
{
    return m_ui->usernameLineEdit->text().trimmed();
}

QString MerHardwareDeviceWizardGeneralPage::freePorts() const
{
    return m_ui->freePortsLineEdit->text().trimmed();
}

int MerHardwareDeviceWizardGeneralPage::sshPort() const
{
    return m_ui->sshPortSpinBox->value();
}

int MerHardwareDeviceWizardGeneralPage::timeout() const
{
    return m_ui->timeoutSpinBox->value();
}

////////////////////////////////////////////////////////////////////////////////////////////////

MerHardwareDeviceWizardKeyPage::MerHardwareDeviceWizardKeyPage(QWidget *parent)
    : QWizardPage(parent)
    , m_ui(new Ui::MerHardwareDeviceWizardKeyPage),
      m_isIdle(true)
{
    m_ui->setupUi(this);
    setTitle(tr("Key Public Key Deployment"));
    m_ui->sshCheckBox->setChecked(true);
    static QRegExp regExp(tr("MerSDK"));
    QList<MerSdk*> sdks = MerSdkManager::instance()->sdks();
    foreach (const MerSdk *s, sdks) {
        m_ui->merSdkComboBox->addItem(s->virtualMachineName());
        if (regExp.indexIn(s->virtualMachineName()) != -1) {
            //preselect sdk
            m_ui->merSdkComboBox->setCurrentIndex(m_ui->merSdkComboBox->count()-1);
        }
    }
    connect(m_ui->merSdkComboBox,SIGNAL(currentIndexChanged(QString)),this,SLOT(handleSdkVmChanged(QString)));
    connect(m_ui->testButton, SIGNAL(clicked()), SLOT(handleTestConnectionClicked()));
    connect(m_ui->passwordLineEdit, SIGNAL(textChanged(QString)), SIGNAL(completeChanged()));
}

void MerHardwareDeviceWizardKeyPage::initializePage()
{
   const MerHardwareDeviceWizard* wizard = qobject_cast<MerHardwareDeviceWizard*>(this->wizard());
   QTC_ASSERT(wizard,return);

   m_ui->hostLabelEdit->setText(wizard->hostName());
   m_ui->usernameLabelEdit->setText(wizard->userName());
   m_ui->sshPortLabelEdit->setText(QString::number(wizard->sshPort()));
   m_ui->connectionLabelEdit->setText(tr("Not connected"));
   handleSdkVmChanged(m_ui->merSdkComboBox->currentText());
}

void MerHardwareDeviceWizardKeyPage::handleSdkVmChanged(const QString &vmName)
{
    MerSdk* sdk = MerSdkManager::instance()->sdk(vmName);
    QTC_ASSERT(sdk,return);
    const MerHardwareDeviceWizard* wizard = qobject_cast<MerHardwareDeviceWizard*>(this->wizard());
    QTC_ASSERT(wizard,return);
    QString index(QLatin1String("/ssh/private_keys/%1/"));
    //TODO: fix me
    QString sshKeyPath(QDir::toNativeSeparators(sdk->sharedConfigPath() +
                       index.arg(wizard->configurationName()).replace(QLatin1String(" "),QLatin1String("_")) +
                       wizard->userName()));
    m_ui->privateSshKeyLabelEdit->setText(sshKeyPath);
    m_ui->publicSshKeyLabelEdit->setText(sshKeyPath + QLatin1String(".pub"));
}

QString MerHardwareDeviceWizardKeyPage::privateKeyFilePath() const
{
    return QDir::fromNativeSeparators(m_ui->privateSshKeyLabelEdit->text());
}

QString MerHardwareDeviceWizardKeyPage::publicKeyFilePath() const
{
    return QDir::fromNativeSeparators(m_ui->publicSshKeyLabelEdit->text());
}

void MerHardwareDeviceWizardKeyPage::handleTestConnectionClicked()
{
    m_isIdle = false;
    completeChanged();
    const MerHardwareDeviceWizard* wizard = qobject_cast<MerHardwareDeviceWizard*>(this->wizard());
    QTC_ASSERT(wizard,return);
    QSsh::SshConnectionParameters sshParams;
    sshParams.host = wizard->hostName();
    sshParams.userName = wizard->userName();
    sshParams.port = wizard->sshPort();
    sshParams.timeout = wizard->timeout();
    sshParams.authenticationType = QSsh::SshConnectionParameters::AuthenticationTypePassword;
    sshParams.password = wizard->password();
    m_ui->connectionLabelEdit->setText(tr("Connecting to machine %1 ...").arg(wizard->hostName()));
    m_ui->testButton->setEnabled(false);
    m_ui->connectionLabelEdit->setText(MerConnectionManager::instance()->testConnection(sshParams));
    m_ui->testButton->setEnabled(true);
    m_isIdle = true;
    completeChanged();
}

bool MerHardwareDeviceWizardKeyPage::isNewSshKeysRquired() const
{
    return m_ui->sshCheckBox->isChecked();
}

QString MerHardwareDeviceWizardKeyPage::password() const
{
    return m_ui->passwordLineEdit->text().trimmed();
}

QString MerHardwareDeviceWizardKeyPage::sharedSshPath() const
{
    MerSdk* sdk = MerSdkManager::instance()->sdk(m_ui->merSdkComboBox->currentText());
    QTC_ASSERT(sdk,return QString());
    return sdk->sharedConfigPath();
}

bool MerHardwareDeviceWizardKeyPage::isComplete() const
{
    return !password().isEmpty() && m_isIdle;
}

}
}
