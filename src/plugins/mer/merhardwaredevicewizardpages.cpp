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

#include "merhardwaredevicewizardpages.h"
#include "ui_merhardwaredevicewizardselectionpage.h"
#include "ui_merhardwaredevicewizardsetuppage.h"

#include "merconnectionmanager.h"
#include "merconstants.h"
#include "merhardwaredevicewizard.h"
#include "mersdkmanager.h"
#include "mervirtualboxmanager.h"

#include <projectexplorer/devicesupport/devicemanager.h>
#include <ssh/sshremoteprocessrunner.h>
#include <utils/portlist.h>
#include <utils/qtcassert.h>

#include <QDir>

using namespace ProjectExplorer;
using namespace QSsh;
using namespace Utils;

namespace Mer {
namespace Internal {


MerHardwareDeviceWizardSelectionPage::MerHardwareDeviceWizardSelectionPage(QWidget *parent)
    : QWizardPage(parent)
    , m_ui(new Ui::MerHardwareDeviceWizardSelectionPage)
    , m_architecture(Abi::UnknownArchitecture)
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

    m_ui->connectionTestLabel->setText(tr("Not connected"));

    connect(m_ui->hostNameLineEdit, &QLineEdit::textChanged,
            this, &MerHardwareDeviceWizardSelectionPage::completeChanged);
    connect(m_ui->usernameLineEdit, &QLineEdit::textChanged,
            this, &MerHardwareDeviceWizardSelectionPage::completeChanged);
    connect(m_ui->passwordLineEdit, &QLineEdit::textChanged,
            this, &MerHardwareDeviceWizardSelectionPage::completeChanged);

    m_ui->testButton->setEnabled(false);
    connect(m_ui->passwordLineEdit, &QLineEdit::textChanged,
            this, [this](const QString &text) {
              m_ui->testButton->setEnabled(!text.isEmpty());
            });
    connect(m_ui->testButton, &QPushButton::clicked,
            this, &MerHardwareDeviceWizardSelectionPage::handleTestConnectionClicked);
    connect(m_ui->passwordLineEdit, &QLineEdit::returnPressed,
            this, &MerHardwareDeviceWizardSelectionPage::handleTestConnectionClicked);
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

Abi::Architecture MerHardwareDeviceWizardSelectionPage::architecture() const
{
    return m_architecture;
}

QString MerHardwareDeviceWizardSelectionPage::deviceName() const
{
    return m_deviceName;
}

void MerHardwareDeviceWizardSelectionPage::handleTestConnectionClicked()
{
    // Without this focus would be moved to the "Cancel" button
    wizard()->setFocus();

    m_ui->testButton->setEnabled(false);
    m_isIdle = false;
    completeChanged();

    SshConnectionParameters sshParams;
    sshParams.setHost(hostName());
    sshParams.setUserName(userName());
    sshParams.setPort(sshPort());
    sshParams.timeout = timeout();
    sshParams.authenticationType = SshConnectionParameters::AuthenticationTypePassword;
    sshParams.setPassword(password());

    m_ui->connectionTestLabel->setText(tr("Connecting to machine %1 ...").arg(hostName()));
    m_ui->connectionTestLabel->setText(MerConnectionManager::testConnection(sshParams,
                &m_connectionTestOk));
    if (!m_connectionTestOk)
        goto end;

    m_ui->connectionTestLabel->setText(tr("Detecting device properties..."));
    m_architecture = detectArchitecture(sshParams, &m_connectionTestOk);
    m_deviceName = detectDeviceName(sshParams, &m_connectionTestOk);
    if (!m_connectionTestOk) {
        m_ui->connectionTestLabel->setText(tr("Could not autodetect device properties"));
        goto end;
    }

    m_ui->connectionTestLabel->setText(tr("Connected %1 device").arg(m_deviceName));

end:
    m_ui->testButton->setEnabled(true);
    if (!m_connectionTestOk) {
      m_ui->passwordLineEdit->setFocus(Qt::TabFocusReason);
    }
    m_isIdle = true;
    completeChanged();
}

Abi::Architecture MerHardwareDeviceWizardSelectionPage::detectArchitecture(
        const SshConnectionParameters &sshParams, bool *ok)
{
    if (!*ok) {
        return Abi::UnknownArchitecture;
    }

    SshRemoteProcessRunner runner;
    QEventLoop loop;
    connect(&runner, &SshRemoteProcessRunner::connectionError,
            &loop, &QEventLoop::quit);
    connect(&runner, &SshRemoteProcessRunner::processClosed,
            &loop, &QEventLoop::quit);
    runner.run("uname --machine", sshParams);
    loop.exec();

    if (runner.lastConnectionError() != SshNoError
            || runner.processExitStatus() != SshRemoteProcess::NormalExit
            || runner.processExitCode() != 0) {
        *ok = false;
        qWarning() << "Failed to execute uname on target";
        return Abi::UnknownArchitecture;
    }

    const QString output = QString::fromLatin1(runner.readAllStandardOutput()).trimmed();
    if (output.isEmpty()) {
        *ok = false;
        qWarning() << "Empty output from uname executed on target";
        return Abi::UnknownArchitecture;
    }

    // Does not seem ideal, but works well
    Abi::Architecture architecture =
        Abi::abiFromTargetTriplet(output).architecture();
    if (architecture == Abi::UnknownArchitecture) {
        *ok = false;
        qWarning() << "Could not parse architecture from uname executed on target (output:"
            << output << ")";
        return Abi::UnknownArchitecture;
    }

    *ok = true;
    return architecture;
}

QString MerHardwareDeviceWizardSelectionPage::detectDeviceName(
        const SshConnectionParameters &sshParams, bool *ok)
{
    if (!*ok) {
        return QString();
    }

    SshRemoteProcessRunner runner;
    QEventLoop loop;
    connect(&runner, &SshRemoteProcessRunner::connectionError,
            &loop, &QEventLoop::quit);
    connect(&runner, &SshRemoteProcessRunner::processClosed,
            &loop, &QEventLoop::quit);
    runner.run("dbus-send --print-reply=literal --system --type=method_call --dest=org.nemo.ssu "
               "/org/nemo/ssu org.nemo.ssu.displayName int32:1", sshParams);
    loop.exec();

    if (runner.lastConnectionError() != SshNoError
            || runner.processExitStatus() != SshRemoteProcess::NormalExit
            || runner.processExitCode() != 0) {
        *ok = false;
        qWarning() << "Failed to query device model name on target";
        return QString();
    }

    const QString deviceName = QString::fromLatin1(runner.readAllStandardOutput()).trimmed();
    if (deviceName.isEmpty()) {
        *ok = false;
        qWarning() << "Empty output from querying device model name on target";
        return QString();
    }

    *ok = true;
    return deviceName;
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

    static QRegExp regExp(tr("Build Engine"));
    QList<MerSdk*> sdks = MerSdkManager::sdks();
    foreach (const MerSdk *s, sdks) {
        m_ui->merSdkComboBox->addItem(s->virtualMachineName());
        if (regExp.indexIn(s->virtualMachineName()) != -1) {
            //preselect sdk
            m_ui->merSdkComboBox->setCurrentIndex(m_ui->merSdkComboBox->count()-1);
        }
    }

    connect(m_ui->configLineEdit, &QLineEdit::textChanged,
            this, &MerHardwareDeviceWizardSetupPage::completeChanged);
    connect(m_ui->merSdkComboBox,
            static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::currentIndexChanged),
            this,
            &MerHardwareDeviceWizardSetupPage::handleSdkVmChanged);
}

void MerHardwareDeviceWizardSetupPage::initializePage()
{
   const MerHardwareDeviceWizard* wizard = qobject_cast<MerHardwareDeviceWizard*>(this->wizard());
   QTC_ASSERT(wizard, return);

   const QString arch = Abi::toString(wizard->architecture()).toUpper();

   QString preferredName = QStringLiteral("%1 (%2)").arg(wizard->deviceName()).arg(arch);
   int i = 1;
   QString tryName = preferredName;
   while (DeviceManager::instance()->hasDevice(tryName))
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
    MerSdk* sdk = MerSdkManager::sdk(vmName);
    QTC_ASSERT(sdk, return);
    const MerHardwareDeviceWizard* wizard = qobject_cast<MerHardwareDeviceWizard*>(this->wizard());
    QTC_ASSERT(wizard, return);
    QString index(QLatin1String("/ssh/private_keys/%1/"));
    //TODO: fix me
    QString sshKeyPath(QDir::toNativeSeparators(sdk->sharedConfigPath() +
                       index.arg(wizard->configurationName()).replace(QLatin1Char(' '), QLatin1Char('_')) +
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
    MerSdk* sdk = MerSdkManager::sdk(m_ui->merSdkComboBox->currentText());
    QTC_ASSERT(sdk, return QString());
    return sdk->sharedConfigPath();
}

bool MerHardwareDeviceWizardSetupPage::isComplete() const
{
    return !configName().isEmpty()
        && !freePorts().isEmpty()
        && !DeviceManager::instance()->hasDevice(configName());
}

}
}
