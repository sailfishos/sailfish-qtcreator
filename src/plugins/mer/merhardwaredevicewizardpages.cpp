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
#include "merlogging.h"
#include "mersdkmanager.h"
#include "mervirtualboxmanager.h"

#include <projectexplorer/devicesupport/devicemanager.h>
#include <ssh/sshremoteprocessrunner.h>
#include <utils/portlist.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QInputDialog>
#include <QMessageBox>
#include <QVersionNumber>

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

    connect(m_ui->testButton, &QPushButton::clicked,
            this, &MerHardwareDeviceWizardSelectionPage::handleTestConnectionClicked);
}

bool MerHardwareDeviceWizardSelectionPage::isComplete() const
{
    return !hostName().isEmpty()
            && !userName().isEmpty()
            && m_isIdle
            && m_connectionTestOk
            && m_sdkClientToolsInstalled;
}

QString MerHardwareDeviceWizardSelectionPage::hostName() const
{
    return m_ui->hostNameLineEdit->text().trimmed();
}

QString MerHardwareDeviceWizardSelectionPage::userName() const
{
    return m_ui->usernameLineEdit->text().trimmed();
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
    sshParams.authenticationType = SshConnectionParameters::AuthenticationTypeAll;

    QString errorMessage;
    m_ui->connectionTestLabel->setText(tr("Connecting to machine %1 ...").arg(hostName()));
    m_ui->connectionTestLabel->setText(MerConnectionManager::testConnection(sshParams,
                &m_connectionTestOk));
    if (!m_connectionTestOk)
        goto end;

    m_ui->connectionTestLabel->setText(tr("Detecting device properties..."));
    m_architecture = detectArchitecture(sshParams, &m_connectionTestOk, &errorMessage);
    m_deviceName = detectDeviceName(sshParams, &m_connectionTestOk, &errorMessage);
    if (!m_connectionTestOk) {
        m_ui->connectionTestLabel->setText(tr("Could not autodetect device properties"));
        showErrorMessageDialog(tr("Could not autodetect device properties."), errorMessage);
        goto end;
    }

    m_ui->connectionTestLabel->setText(tr(
        "Detecting software packages required for proper SDK operation..."));
    m_sdkClientToolsInstalled = detectSdkClientTools(sshParams, &m_connectionTestOk, &errorMessage);
    if (!m_connectionTestOk) {
        m_ui->connectionTestLabel->setText(tr("Could not autodetect installed software packages"));
        showErrorMessageDialog(tr(
            "Could not autodetect installed software packages required for proper SDK operation."), errorMessage);
        goto end;
    }
    if (!m_sdkClientToolsInstalled) {
        m_ui->connectionTestLabel->setText(tr(
            "Software packages required for proper SDK operation are missing"));
        QString password;
        if (askInstallSdkClientTools(&password)) {
            m_ui->connectionTestLabel->setText(tr("Installing software packages..."));
            m_sdkClientToolsInstalled = installSdkClientTools(sshParams, password, &errorMessage);
            if (!m_sdkClientToolsInstalled) {
                m_ui->connectionTestLabel->setText(tr("Software package installation failed"));
                showErrorMessageDialog(
                    tr("Installation of software packages required for proper SDK operation failed."), errorMessage);
                goto end;
            }
        } else {
            goto end;
        }
    }

    m_ui->connectionTestLabel->setText(tr("Connected %1 device").arg(m_deviceName));

end:
    m_ui->testButton->setEnabled(true);
    m_isIdle = true;
    completeChanged();
}

Abi::Architecture MerHardwareDeviceWizardSelectionPage::detectArchitecture(
        const SshConnectionParameters &sshParams, bool *ok, QString *errorMessage)
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

    if (!runner.lastConnectionErrorString().isEmpty()
            || runner.processExitStatus() != SshRemoteProcess::NormalExit
            || runner.processExitCode() != 0) {
        *ok = false;
        *errorMessage = tr("Failed to detect architecture: Command 'uname' could not be executed.");
        return Abi::UnknownArchitecture;
    }

    const QString output = QString::fromLatin1(runner.readAllStandardOutput()).trimmed();
    if (output.isEmpty()) {
        *ok = false;
        *errorMessage = tr("Failed to detect architecture: Empty output from 'uname' command.");
        return Abi::UnknownArchitecture;
    }

    // Does not seem ideal, but works well
    Abi::Architecture architecture =
        Abi::abiFromTargetTriplet(output).architecture();
    if (architecture == Abi::UnknownArchitecture) {
        *ok = false;
        *errorMessage = tr("Failed to detect architecture: "
            "Could not parse architecture from 'uname' command, result: %1").arg(output);
        return Abi::UnknownArchitecture;
    }

    *ok = true;
    return architecture;
}

QString MerHardwareDeviceWizardSelectionPage::detectDeviceName(
        const SshConnectionParameters &sshParams, bool *ok, QString *errorMessage)
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

    if (!runner.lastConnectionErrorString().isEmpty()
            || runner.processExitStatus() != SshRemoteProcess::NormalExit
            || runner.processExitCode() != 0) {
        *ok = false;
        *errorMessage = tr("Failed to query device model name on device.");
        return QString();
    }

    const QString deviceName = QString::fromLatin1(runner.readAllStandardOutput()).trimmed();
    if (deviceName.isEmpty()) {
        *ok = false;
        *errorMessage = tr("Failed to query device model name on device: Output was empty.");
        return QString();
    }

    *ok = true;
    return deviceName;
}

bool MerHardwareDeviceWizardSelectionPage::detectSdkClientTools(
        const SshConnectionParameters &sshParams, bool *ok, QString *errorMessage)
{
    if (!*ok) {
        return false;
    }

    SshRemoteProcessRunner runner;
    QEventLoop loop;
    connect(&runner, &SshRemoteProcessRunner::connectionError,
            &loop, &QEventLoop::quit);
    connect(&runner, &SshRemoteProcessRunner::processClosed,
            &loop, &QEventLoop::quit);
    runner.run("sed -n '/^VERSION_ID=/s/^VERSION_ID=\\([0-9]\\+\\.[0-9]\\+\\.[0-9]\\+\\).*/\\1/p'"
               " /etc/os-release", sshParams);
    loop.exec();

    if (!runner.lastConnectionErrorString().isEmpty()
            || runner.processExitStatus() != SshRemoteProcess::NormalExit
            || runner.processExitCode() != 0) {
        *ok = false;
        *errorMessage = tr("Failed to query OS version.");
        return false;
    }

    const QString osVersion = QString::fromLatin1(runner.readAllStandardOutput()).trimmed();
    if (osVersion.isEmpty()) {
        *ok = false;
        *errorMessage = tr("Failed to query OS version: Output was empty.");
        return false;
    }

    if (QVersionNumber::fromString(osVersion) < QVersionNumber(3, 2, 1)) {
        *ok = true;
        qCDebug(Log::mer) << "Sailfish OS" << osVersion << "assume that packages are installed";
        return true;
    }

    runner.run("rpm -q patterns-sailfish-sdk-client-tools", sshParams);
    loop.exec();

    // rpm returns 0 if package is found and 1 if it is not found
    if (!runner.lastConnectionErrorString().isEmpty()
            || runner.processExitStatus() != SshRemoteProcess::NormalExit
            || (runner.processExitCode() != 0 && runner.processExitCode() != 1)) {
        *ok = false;
        *errorMessage = tr("Failed to query install status of 'patterns-sailfish-sdk-client-tools'.");
        return false;
    }

    if (runner.processExitCode() == 1) {
        *ok = true;
        qCDebug(Log::mer) << "patterns-sailfish-sdk-client-tools is not installed";
        return false;
    }

    qCDebug(Log::mer) << "patterns-sailfish-sdk-client-tools is installed";
    *ok = true;
    return true;
}

bool MerHardwareDeviceWizardSelectionPage::askInstallSdkClientTools(QString *password)
{
    QInputDialog passwordDialog;
    passwordDialog.setWindowTitle(tr("Software package installation needed"));
    passwordDialog.setLabelText(tr("Enter developer mode password to install "
                "software packages required for proper SDK operation."));
    passwordDialog.setInputMode(QInputDialog::TextInput);
    passwordDialog.setTextEchoMode(QLineEdit::Password);
    passwordDialog.setCancelButtonText(tr("Abort"));
    passwordDialog.setOkButtonText(tr("Install"));
    passwordDialog.exec();
    *password = passwordDialog.textValue();
    return passwordDialog.result() == QInputDialog::Accepted;
}

bool MerHardwareDeviceWizardSelectionPage::installSdkClientTools(
        const SshConnectionParameters &sshParams, const QString &password, QString *errorMessage)
{
    SshRemoteProcessRunner runner;
    QEventLoop loop;
    QByteArray passwordData(password.toLatin1());
    passwordData.append("\n");
    connect(&runner, &SshRemoteProcessRunner::connectionError,
            &loop, &QEventLoop::quit);
    connect(&runner, &SshRemoteProcessRunner::processClosed,
            &loop, &QEventLoop::quit);
    connect(&runner, &SshRemoteProcessRunner::processStarted,
            [&runner, &passwordData] { runner.writeDataToProcess(passwordData); });

    // Try to install
    runner.run("devel-su pkcon --plain --noninteractive install "
               "patterns-sailfish-sdk-client-tools", sshParams);
    loop.exec();

    if (runner.lastConnectionErrorString().isEmpty()
            && runner.processExitStatus() == SshRemoteProcess::NormalExit
            && runner.processExitCode() == 0) {
        qCDebug(Log::mer) << "patterns-sailfish-sdk-client-tools was installed";
        return true;
    }

    qCDebug(Log::mer) << "Failed to install patterns-sailfish-sdk-client-tools,"
                      << "trying again after refreshing";

    // Install failed, refresh and try again
    runner.run("devel-su pkcon --plain --noninteractive refresh", sshParams);
    loop.exec();

    if (!runner.lastConnectionErrorString().isEmpty()
            || runner.processExitStatus() != SshRemoteProcess::NormalExit
            || runner.processExitCode() != 0) {
        *errorMessage = tr("Refreshing list of packages failed. "
                           "Check your password and internet connection on device and try again.");
        return false;
    }

    runner.run("devel-su pkcon --plain --noninteractive install "
               "patterns-sailfish-sdk-client-tools", sshParams);
    loop.exec();

    if (!runner.lastConnectionErrorString().isEmpty()
            || runner.processExitStatus() != SshRemoteProcess::NormalExit
            || runner.processExitCode() != 0) {
        *errorMessage = tr("Installation of software packages failed. "
                           "Check internet connection on device and try again.");
        return false;
    }

    qCDebug(Log::mer) << "patterns-sailfish-sdk-client-tools was installed";
    return true;
}

void MerHardwareDeviceWizardSelectionPage::showErrorMessageDialog(const QString &error, const QString &message)
{
    QMessageBox errorDialog(this);
    errorDialog.setWindowTitle(tr("Connection test failed"));
    errorDialog.setText(error);
    errorDialog.setInformativeText(message);
    errorDialog.setIcon(QMessageBox::Critical);
    errorDialog.exec();
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
   QTC_ASSERT(wizard,return);

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
    QTC_ASSERT(sdk,return);
    const MerHardwareDeviceWizard* wizard = qobject_cast<MerHardwareDeviceWizard*>(this->wizard());
    QTC_ASSERT(wizard,return);
    QString index(QLatin1String("/ssh/private_keys/%1/"));
    //TODO: fix me
    QString sshKeyPath(QDir::toNativeSeparators(sdk->sharedConfigPath() +
                       index.arg(wizard->configurationName()).replace(QLatin1Char(' '),QLatin1Char('_')) +
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
    QTC_ASSERT(sdk,return QString());
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
