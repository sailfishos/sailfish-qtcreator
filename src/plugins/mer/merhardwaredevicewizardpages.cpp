/****************************************************************************
**
** Copyright (C) 2012-2019 Jolla Ltd.
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

#include "merhardwaredevicewizardpages.h"
#include "ui_merhardwaredevicewizardselectionpage.h"
#include "ui_merhardwaredevicewizardconnectiontestpage.h"
#include "ui_merhardwaredevicewizardsetuppage.h"

#include "merconnectionmanager.h"
#include "merconstants.h"
#include "merdevice.h"
#include "merhardwaredevicewizard.h"
#include "merlogging.h"
#include "mersigninguserselectiondialog.h"
#include "mersettings.h"

#include <sfdk/buildengine.h>
#include <sfdk/device.h>
#include <sfdk/sdk.h>
#include <sfdk/utils.h>

#include <coreplugin/messagemanager.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <ssh/sshremoteprocessrunner.h>
#include <utils/algorithm.h>
#include <utils/pathchooser.h>
#include <utils/portlist.h>
#include <utils/progressindicator.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QAbstractButton>
#include <QComboBox>
#include <QDir>
#include <QInputDialog>
#include <QMessageBox>
#include <QPushButton>
#include <QTimer>
#include <QVersionNumber>

using namespace ProjectExplorer;
using namespace QSsh;
using namespace Sfdk;
using namespace Utils;

namespace Mer {
namespace Internal {

/*!
 * \class MerHardwareDeviceWizardSelectionPage
 * \internal
 */

MerHardwareDeviceWizardSelectionPage::MerHardwareDeviceWizardSelectionPage(QWidget *parent)
    : QWizardPage(parent)
    , m_ui(new Ui::MerHardwareDeviceWizardSelectionPage)
{
    m_ui->setupUi(this);
    setTitle(tr("Connection"));
    setSubTitle(QLatin1String(" ")); // For Qt bug (background color)

    m_ui->hostNameLineEdit->setText(QLatin1String("192.168.2.15"));
    m_ui->usernameLineEdit->setText(QLatin1String(Constants::MER_DEVICE_DEFAULTUSER));

    m_ui->sshPortSpinBox->setMinimum(1);
    m_ui->sshPortSpinBox->setMaximum(65535);
    m_ui->sshPortSpinBox->setValue(22);

    m_ui->timeoutSpinBox->setMinimum(1);
    m_ui->timeoutSpinBox->setMaximum(65535);
    m_ui->timeoutSpinBox->setValue(10);

    connect(m_ui->hostNameLineEdit, &QLineEdit::textChanged,
            this, &MerHardwareDeviceWizardSelectionPage::completeChanged);
    connect(m_ui->usernameLineEdit, &QLineEdit::textChanged,
            this, &MerHardwareDeviceWizardSelectionPage::completeChanged);
}

void MerHardwareDeviceWizardSelectionPage::setDevice(const MerHardwareDevice::Ptr &device)
{
    m_device = device;
}

bool MerHardwareDeviceWizardSelectionPage::isComplete() const
{
    return !hostName().isEmpty()
        && !userName().isEmpty();
}

bool MerHardwareDeviceWizardSelectionPage::validatePage()
{
    SshConnectionParameters sshParams = m_device->sshParameters();
    sshParams.setHost(hostName());
    sshParams.setUserName(userName());
    sshParams.setPort(sshPort());
    sshParams.timeout = timeout();
    m_device->setSshParameters(sshParams);
    return true;
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

/*!
 * \class MerHardwareDeviceWizardConnectionTestPage
 * \internal
 */

MerHardwareDeviceWizardConnectionTestPage::MerHardwareDeviceWizardConnectionTestPage(QWidget *parent)
    : QWizardPage(parent)
    , m_ui(new Ui::MerHardwareDeviceWizardConnectionTestPage)
    , m_architecture(Abi::UnknownArchitecture)
    , m_isIdle(true)
    , m_connectionTestOk(false)
{
    m_ui->setupUi(this);
    setTitle(tr("Device preparation"));
    setSubTitle(QLatin1String(" "));

    m_ui->connectionTestLabel->setText(tr("Not connected"));
}

void MerHardwareDeviceWizardConnectionTestPage::setDevice(const MerHardwareDevice::Ptr &device)
{
    m_device = device;
}

void MerHardwareDeviceWizardConnectionTestPage::initializePage()
{
    QTimer::singleShot(0, this, &MerHardwareDeviceWizardConnectionTestPage::testConnection);
}

bool MerHardwareDeviceWizardConnectionTestPage::isComplete() const
{
    return m_isIdle
        && m_connectionTestOk
        && m_sdkClientToolsInstalled;
}

bool MerHardwareDeviceWizardConnectionTestPage::validatePage()
{
    m_device->setArchitecture(m_architecture);

    const QString arch = Abi::toString(m_architecture).toUpper();
    QString preferredName = QStringLiteral("%1 (%2)").arg(m_deviceName).arg(arch);
    int i = 1;
    QString tryName = preferredName;
    while (DeviceManager::instance()->hasDevice(tryName))
        tryName = preferredName + QString::number(++i);

    m_device->setDisplayName(tryName);

    return true;
}

void MerHardwareDeviceWizardConnectionTestPage::testConnection()
{
    // Without this focus would be moved to the "Cancel" button
    wizard()->setFocus();

    m_isIdle = false;
    completeChanged();

    // QWizard will revert this next time completeChanged is emited
    wizard()->button(QWizard::BackButton)->setEnabled(false);

    const SshConnectionParameters sshParams = m_device->sshParameters();

    QString errorMessage;
    m_ui->connectionTestLabel->setText(tr("Connecting to machine %1 ...").arg(sshParams.host()));
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

    m_ui->connectionTestLabel->setText(tr("Prepared %1 device").arg(m_deviceName));

end:
    m_isIdle = true;
    completeChanged();
}

Abi::Architecture MerHardwareDeviceWizardConnectionTestPage::detectArchitecture(
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

QString MerHardwareDeviceWizardConnectionTestPage::detectDeviceName(
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

bool MerHardwareDeviceWizardConnectionTestPage::detectSdkClientTools(
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

bool MerHardwareDeviceWizardConnectionTestPage::askInstallSdkClientTools(QString *password)
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

bool MerHardwareDeviceWizardConnectionTestPage::installSdkClientTools(
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

void MerHardwareDeviceWizardConnectionTestPage::showErrorMessageDialog(const QString &error, const QString &message)
{
    QMessageBox errorDialog(this);
    errorDialog.setWindowTitle(tr("Failed to prepare device"));
    errorDialog.setText(error);
    errorDialog.setInformativeText(message);
    errorDialog.setIcon(QMessageBox::Critical);
    errorDialog.exec();
}

/*!
 * \class MerHardwareDeviceWizardSetupPage
 * \internal
 */

MerHardwareDeviceWizardSetupPage::MerHardwareDeviceWizardSetupPage(QWidget *parent)
    : QWizardPage(parent)
    , m_ui(new Ui::MerHardwareDeviceWizardSetupPage)
{
    m_ui->setupUi(this);
    setTitle(tr("Configuration"));
    setSubTitle(QLatin1String(" "));

    m_ui->freePortsLineEdit->setText(QLatin1String("10000-10100"));

    connect(m_ui->configLineEdit, &QLineEdit::textChanged,
            this, &MerHardwareDeviceWizardSetupPage::completeChanged);
}

void MerHardwareDeviceWizardSetupPage::setDevice(const MerHardwareDevice::Ptr &device)
{
    m_device = device;
}

void MerHardwareDeviceWizardSetupPage::initializePage()
{
    m_ui->configLineEdit->setText(m_device->displayName());
    const SshConnectionParameters sshParams = m_device->sshParameters();
    m_ui->hostLabelEdit->setText(sshParams.host());
    m_ui->usernameLabelEdit->setText(sshParams.userName());
    m_ui->sshPortLabelEdit->setText(QString::number(sshParams.port()));
}

QString MerHardwareDeviceWizardSetupPage::configName() const
{
    return m_ui->configLineEdit->text().trimmed();
}

QString MerHardwareDeviceWizardSetupPage::freePorts() const
{
    return m_ui->freePortsLineEdit->text().trimmed();
}

bool MerHardwareDeviceWizardSetupPage::isComplete() const
{
    return !configName().isEmpty()
        && !freePorts().isEmpty()
        && !DeviceManager::instance()->hasDevice(configName());
}

bool MerHardwareDeviceWizardSetupPage::validatePage()
{
    m_device->setDisplayName(configName());
    m_device->setFreePorts(PortList::fromString(freePorts()));
    return true;
}

/*!
 * \class MerHardwareDeviceWizardPackageKeyDeploymentPage
 * \internal
 */

MerHardwareDeviceWizardPackageKeyDeploymentPage::MerHardwareDeviceWizardPackageKeyDeploymentPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle(tr("RPM Key Deployment"));
    setSubTitle(QLatin1String(" "));

    auto infoLabel = new QLabel;
    infoLabel->setText(tr("Set up your device for package signature verification.\n"
            "Depending on your device configuration, manual confirmation may be "
            "skipped for deployment of packages signed with trusted keys."));
    infoLabel->setWordWrap(true);

    m_keyComboBox = new QComboBox;
    m_keyComboBox->setToolTip(tr("Select a trusted GnuPG public key"));

    auto deployKeyButton = new QPushButton(tr("Deploy Key"));
    connect(deployKeyButton, &QPushButton::clicked,
            this, &MerHardwareDeviceWizardPackageKeyDeploymentPage::deployKey);

    m_deployStateLabel = new QLabel;
    m_deployStateLabel->hide();
    m_progressIndicator = new ProgressIndicator(ProgressIndicatorSize::Small);
    m_progressIndicator->hide();

    auto mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(infoLabel);
    auto errorLayout = new QHBoxLayout(this);
    mainLayout->addLayout(errorLayout);

    auto keyLayout = new QHBoxLayout(this);
    keyLayout->addWidget(new QLabel(tr("Public key:")));
    keyLayout->addWidget(m_keyComboBox);
    mainLayout->addLayout(keyLayout);

    auto deployLayout = new QHBoxLayout(this);
    deployLayout->addWidget(deployKeyButton);
    deployLayout->addWidget(m_deployStateLabel);
    deployLayout->addWidget(m_progressIndicator);
    deployLayout->addStretch();
    mainLayout->addLayout(deployLayout);

    QString gpgErrorString;
    if (!Sfdk::isGpgAvailable(&gpgErrorString)) {
        auto errorLabel = new QLabel;
        auto signErrorIconLabel = new QLabel;
        errorLayout->addWidget(signErrorIconLabel);
        errorLayout->addWidget(errorLabel);
        errorLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));
        errorLabel->setText(gpgErrorString);
        signErrorIconLabel->setPixmap(Icons::WARNING.pixmap());
        signErrorIconLabel->setVisible(true);

        deployKeyButton->setEnabled(false);
        m_keyComboBox->setEnabled(false);
        return;
    }
    QList<GpgKeyInfo> availableKeys;
    bool ok;
    QString error;
    execAsynchronous(std::tie(ok, availableKeys, error), Sfdk::availableGpgKeys);
    if (!ok)
        Core::MessageManager::write(error, Core::MessageManager::Flash);

    const QStringList items = Utils::transform(availableKeys, &GpgKeyInfo::toString);
    m_keyComboBox->addItems(items);
    m_keyComboBox->setCurrentText(MerSettings::signingUser().toString());
}

void MerHardwareDeviceWizardPackageKeyDeploymentPage::setDevice(const MerHardwareDevice::Ptr &device)
{
    m_device = device;
}

void MerHardwareDeviceWizardPackageKeyDeploymentPage::deployKey()
{
    QTC_ASSERT(m_device, return);

    m_progressIndicator->show();
    m_deployStateLabel->hide();

    auto onDeployFinished = [this](bool ok) {
        m_progressIndicator->hide();
        m_deployStateLabel->setPixmap(ok ? Icons::OK.pixmap() : Icons::BROKEN.pixmap());
        m_deployStateLabel->show();
    };

    const QModelIndex currentModelIndex = m_keyComboBox->model()->index(
            m_keyComboBox->currentIndex(), 0);
    if (!currentModelIndex.isValid()) {
        onDeployFinished(false);
        return;
    }
    const auto signingUser = GpgKeyInfo::fromString(m_keyComboBox->currentText());
    QTC_CHECK(signingUser.isValid());

    const QString sdkId = MerHardwareDevice::toSdkId(m_device->id());
    const auto sdkArch = MerDevice::architecture_cast<Device::Architecture>(m_device->architecture());

    std::unique_ptr<HardwareDevice> newSdkDevice;
    newSdkDevice = std::make_unique<HardwareDevice>(sdkId, sdkArch);
    newSdkDevice->setName(m_device->displayName());
    newSdkDevice->setSshParameters(m_device->sshParameters());

    MerDeviceGpgKeyDeploymentDialog dlg(newSdkDevice.get(), signingUser.fingerprint, this);
    onDeployFinished(dlg.exec() == QDialog::Accepted);
}

/*!
 * \class MerDeviceGpgKeyDeploymentDialog
 * \internal
 */

MerDeviceGpgKeyDeploymentDialog::MerDeviceGpgKeyDeploymentDialog(
    Sfdk::Device *device, const QString &id, QWidget *parent)
    : QProgressDialog(parent)
{
    QTC_ASSERT(device, done = false; return );

    setAutoReset(false);
    setAutoClose(false);
    setMinimumDuration(0);
    setMaximum(1);

    setLabelText(tr("Deploying..."));
    setValue(0);

    connect(this, &MerDeviceGpgKeyDeploymentDialog::canceled,
            this, &MerDeviceGpgKeyDeploymentDialog::handleCanceled);

    bool ok;
    QString errorString;
    execAsynchronous(std::tie(ok, errorString), std::mem_fn(&Device::importPublicGpgKey), device, id);
    QString buttonText;
    const char *textColor;
    if (ok) {
        buttonText = tr("Deployment finished successfully.");
        textColor = "blue";
        setValue(1);
    } else {
        buttonText = errorString;
        textColor = "red";
    }
    setLabelText(QString::fromLatin1("<font color=\"%1\">%2</font>")
            .arg(QLatin1String(textColor), buttonText));
    setCancelButtonText(tr("Close"));
    done = ok;
}

MerDeviceGpgKeyDeploymentDialog *MerDeviceGpgKeyDeploymentDialog::createDialog(
    Sfdk::Device *device, QWidget *parent)
{
    QTC_ASSERT(device, return nullptr);

    const GpgKeyInfo selectedSigningUser = MerSigningUserSelectionDialog::selectSigningUser();
    if (!selectedSigningUser.isValid())
        return nullptr;

    return new MerDeviceGpgKeyDeploymentDialog(device, selectedSigningUser.fingerprint, parent);
}

void MerDeviceGpgKeyDeploymentDialog::handleCanceled()
{
    if (done)
        accept();
    else
        reject();
}

} // Internal
} // Mer
