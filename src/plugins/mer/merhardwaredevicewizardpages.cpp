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
#include "merconstants.h"
#include "mersdkmanager.h"
#include "mervirtualboxmanager.h"
#include "merhardwaredevicewizardpages.h"
#include "merhardwaredevicewizard.h"
#include "merconnectionmanager.h"
#include "ui_merhardwaredevicewizardselectionpage.h"
#include "ui_merhardwaredevicewizardsetuppage.h"
#include <projectexplorer/devicesupport/devicemanager.h>
#include <ssh/sshremoteprocessrunner.h>
#include <utils/qtcassert.h>
#include <utils/portlist.h>
#include <QDir>

namespace Mer {
namespace Internal {


MerHardwareDeviceWizardSelectionPage::MerHardwareDeviceWizardSelectionPage(QWidget *parent)
    : QWizardPage(parent)
    , m_ui(new Ui::MerHardwareDeviceWizardSelectionPage)
    , m_architecture(ProjectExplorer::Abi::UnknownArchitecture)
    , m_isIdle(true)
    , m_connectionTestOk(false)
{
    m_ui->setupUi(this);
    setTitle(tr("Select Device"));

    m_ui->hostNameLineEdit->setText(QLatin1String("192.168.2.15"));
    m_ui->usernameLineEdit->setText(QLatin1String(Constants::MER_DEVICE_DEFAULTUSER));

    m_ui->sshPortSpinBox->setMinimum(1);
    m_ui->sshPortSpinBox->setMaximum(65535);
    m_ui->sshPortSpinBox->setValue(22);

    m_ui->timeoutSpinBox->setMinimum(1);
    m_ui->timeoutSpinBox->setMaximum(65535);
    m_ui->timeoutSpinBox->setValue(10);

    m_ui->connectionLabelEdit->setText(tr("Not connected"));

    connect(m_ui->hostNameLineEdit, SIGNAL(textChanged(QString)), SIGNAL(completeChanged()));
    connect(m_ui->usernameLineEdit, SIGNAL(textChanged(QString)), SIGNAL(completeChanged()));
    connect(m_ui->passwordLineEdit, SIGNAL(textChanged(QString)), SIGNAL(completeChanged()));
    connect(m_ui->testButton, SIGNAL(clicked()), SLOT(handleTestConnectionClicked()));
}

bool MerHardwareDeviceWizardSelectionPage::isComplete() const
{
    return !hostName().isEmpty()
            && !userName().isEmpty()
            && !password().isEmpty()
            && m_isIdle
            && m_connectionTestOk;
}

QString MerHardwareDeviceWizardSelectionPage::hostName() const
{
    return m_ui->hostNameLineEdit->text().trimmed();
}

QString MerHardwareDeviceWizardSelectionPage::userName() const
{
    return m_ui->usernameLineEdit->text().trimmed();
}

QString MerHardwareDeviceWizardSelectionPage::password() const
{
    return m_ui->passwordLineEdit->text().trimmed();
}

int MerHardwareDeviceWizardSelectionPage::sshPort() const
{
    return m_ui->sshPortSpinBox->value();
}

int MerHardwareDeviceWizardSelectionPage::timeout() const
{
    return m_ui->timeoutSpinBox->value();
}

ProjectExplorer::Abi::Architecture MerHardwareDeviceWizardSelectionPage::architecture() const
{
    return m_architecture;
}

void MerHardwareDeviceWizardSelectionPage::handleTestConnectionClicked()
{
    m_ui->testButton->setEnabled(false);
    m_isIdle = false;
    completeChanged();

    QSsh::SshConnectionParameters sshParams;
    sshParams.host = hostName();
    sshParams.userName = userName();
    sshParams.port = sshPort();
    sshParams.timeout = timeout();
    sshParams.authenticationType = QSsh::SshConnectionParameters::AuthenticationTypePassword;
    sshParams.password = password();

    m_ui->connectionLabelEdit->setText(tr("Connecting to machine %1 ...").arg(hostName()));
    m_ui->connectionLabelEdit->setText(MerConnectionManager::instance()->testConnection(sshParams,
                &m_connectionTestOk));
    if (!m_connectionTestOk)
        goto end;

    m_ui->connectionLabelEdit->setText(tr("Detecting device properties..."));
    m_architecture = detectArchitecture(sshParams, &m_connectionTestOk);
    if (!m_connectionTestOk) {
        m_ui->connectionLabelEdit->setText(tr("Could autodetect device properties"));
        goto end;
    }

    m_ui->connectionLabelEdit->setText(tr("Connected"));

end:
    m_ui->testButton->setEnabled(true);
    m_isIdle = true;
    completeChanged();
}

ProjectExplorer::Abi::Architecture MerHardwareDeviceWizardSelectionPage::detectArchitecture(
        const QSsh::SshConnectionParameters &sshParams, bool *ok)
{
    if (!*ok) {
        return ProjectExplorer::Abi::UnknownArchitecture;
    }

    QSsh::SshRemoteProcessRunner runner;
    QEventLoop loop;
    connect(&runner, SIGNAL(connectionError()), &loop, SLOT(quit()));
    connect(&runner, SIGNAL(processClosed(int)), &loop, SLOT(quit()));
    runner.run("uname --machine", sshParams);
    loop.exec();

    if (runner.lastConnectionError() != QSsh::SshNoError
            || runner.processExitStatus() != QSsh::SshRemoteProcess::NormalExit
            || runner.processExitCode() != 0) {
        *ok = false;
        qWarning() << "Failed to execute uname on target";
        return ProjectExplorer::Abi::UnknownArchitecture;
    }

    const QString output = QString::fromLatin1(runner.readAllStandardOutput()).trimmed();
    if (output.isEmpty()) {
        *ok = false;
        qWarning() << "Empty output from uname executed on target";
        return ProjectExplorer::Abi::UnknownArchitecture;
    }

    // Does not seem ideal, but works well
    ProjectExplorer::Abi::Architecture architecture =
        ProjectExplorer::Abi::abiFromTargetTriplet(output).architecture();
    if (architecture == ProjectExplorer::Abi::UnknownArchitecture) {
        *ok = false;
        qWarning() << "Could not parse architecture from uname executed on target (output:"
            << output << ")";
        return ProjectExplorer::Abi::UnknownArchitecture;
    }

    *ok = true;
    return architecture;
}

////////////////////////////////////////////////////////////////////////////////////////////////

MerHardwareDeviceWizardSetupPage::MerHardwareDeviceWizardSetupPage(QWidget *parent)
    : QWizardPage(parent)
    , m_ui(new Ui::MerHardwareDeviceWizardSetupPage)
{
    m_ui->setupUi(this);
    setTitle(tr("Configure Connection"));

    m_ui->freePortsLineEdit->setText(QLatin1String("10000-10100"));

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

    connect(m_ui->configLineEdit, SIGNAL(textChanged(QString)), SIGNAL(completeChanged()));
    connect(m_ui->merSdkComboBox,SIGNAL(currentIndexChanged(QString)),this,SLOT(handleSdkVmChanged(QString)));
}

void MerHardwareDeviceWizardSetupPage::initializePage()
{
   const MerHardwareDeviceWizard* wizard = qobject_cast<MerHardwareDeviceWizard*>(this->wizard());
   QTC_ASSERT(wizard,return);

   const QString arch = ProjectExplorer::Abi::toString(wizard->architecture()).toUpper();

   QString preferredName = tr("SailfishOS Device (%1)").arg(arch);
   int i = 1;
   QString tryName = preferredName;
   while (ProjectExplorer::DeviceManager::instance()->hasDevice(tryName))
       tryName = preferredName + QString::number(++i);
   m_ui->configLineEdit->setText(tryName);

   m_ui->hostLabelEdit->setText(wizard->hostName());
   m_ui->usernameLabelEdit->setText(wizard->userName());
   m_ui->sshPortLabelEdit->setText(QString::number(wizard->sshPort()));
   handleSdkVmChanged(m_ui->merSdkComboBox->currentText());
}

QString MerHardwareDeviceWizardSetupPage::configName() const
{
    return m_ui->configLineEdit->text().trimmed();
}

QString MerHardwareDeviceWizardSetupPage::freePorts() const
{
    return m_ui->freePortsLineEdit->text().trimmed();
}

void MerHardwareDeviceWizardSetupPage::handleSdkVmChanged(const QString &vmName)
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

QString MerHardwareDeviceWizardSetupPage::privateKeyFilePath() const
{
    return QDir::fromNativeSeparators(m_ui->privateSshKeyLabelEdit->text());
}

QString MerHardwareDeviceWizardSetupPage::publicKeyFilePath() const
{
    return QDir::fromNativeSeparators(m_ui->publicSshKeyLabelEdit->text());
}

bool MerHardwareDeviceWizardSetupPage::isNewSshKeysRquired() const
{
    return m_ui->sshCheckBox->isChecked();
}

QString MerHardwareDeviceWizardSetupPage::sharedSshPath() const
{
    MerSdk* sdk = MerSdkManager::instance()->sdk(m_ui->merSdkComboBox->currentText());
    QTC_ASSERT(sdk,return QString());
    return sdk->sharedConfigPath();
}

bool MerHardwareDeviceWizardSetupPage::isComplete() const
{
    return !configName().isEmpty()
        && !freePorts().isEmpty()
        && !ProjectExplorer::DeviceManager::instance()->hasDevice(configName());
}

}
}
