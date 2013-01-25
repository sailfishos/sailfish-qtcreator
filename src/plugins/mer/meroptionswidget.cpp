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

#include "meroptionswidget.h"
#include "ui_meroptionswidget.h"
#include "sdkdetailswidget.h"
#include "mersdkmanager.h"
#include "sdkkitutils.h"
#include "sdktoolchainutils.h"
#include "sdktargetutils.h"
#include "sdkscriptsutils.h"
#include "sdkselectiondialog.h"
#include "mervirtualmachinemanager.h"
#include "virtualboxmanager.h"

#include <utils/fileutils.h>
#include <ssh/sshkeygenerator.h>

#include <QStandardItemModel>
#include <QStandardItem>
#include <QDir>
#include <QDesktopServices>
#include <QUrl>

namespace Mer {
namespace Internal {

const char CONTROLCENTER_URL[] = "http://127.0.0.1:8080/";

MerOptionsWidget::MerOptionsWidget(QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui::MerOptionsWidget)
    , m_detailsWidget(new SdkDetailsWidget(this))
    , m_model(0)
    , m_toolChainUtils(new SdkToolChainUtils)
    , m_targetUtils(new SdkTargetUtils)
    , m_currentSdkIndex(-1)
    , m_currentCommand(None)
{
    m_timer.setSingleShot(true);
    m_timer.setInterval(0);
    connect(&m_timer, SIGNAL(timeout()), SLOT(changeState()));

    connect(m_detailsWidget, SIGNAL(testConnection()), SLOT(testConnection()));
    connect(m_detailsWidget, SIGNAL(updateKits(QStringList)),
            SLOT(onUpdateKitsButtonClicked(QStringList)));
    connect(m_detailsWidget, SIGNAL(deployPublicKey()), SLOT(onDeployPublicKeyButtonClicked()));
    connect(m_detailsWidget, SIGNAL(createSshKey(QString)), SLOT(onCreateSshKey(QString)));

    m_ui->setupUi(this);

    connect(m_targetUtils, SIGNAL(installedTargets(QString,QStringList)),
            SLOT(onInstalledTargets(QString,QStringList)));

    MerVirtualMachineManager *remoteManager = MerVirtualMachineManager::instance();
    connect(remoteManager, SIGNAL(connectionChanged(QString,bool)),
            SLOT(onConnectionChanged(QString,bool)));
    connect(remoteManager, SIGNAL(error(QString,QString)),
            SLOT(onConnectionError(QString,QString)));

    connect(m_ui->sdkComboBox, SIGNAL(currentIndexChanged(int)), SLOT(onSdkChanged(int)));
    connect(m_ui->sdkComboBox, SIGNAL(currentIndexChanged(int)), SIGNAL(updateSearchKeys()));

    connect(m_ui->addButton, SIGNAL(clicked()), SLOT(onAddButtonClicked()));
    connect(m_ui->removeButton, SIGNAL(clicked()), SLOT(onRemoveButtonClicked()));
    connect(m_ui->startVirtualMachineButton, SIGNAL(clicked()),
            SLOT(onStartVirtualMachineButtonClicked()));

    m_ui->launchSDKControlCenterPushButton->setToolTip(tr("Open Mer SDK Control Center in "
                                                          "default web browser"));
    connect(m_ui->launchSDKControlCenterPushButton, SIGNAL(clicked()),
            SLOT(onLaunchSDKControlCenterClicked()));

    m_ui->sdkDetailsWidget->setWidget(m_detailsWidget);
    m_ui->sdkDetailsWidget->setState(Utils::DetailsWidget::NoSummary);
    m_ui->sdkDetailsWidget->setVisible(false);

    m_ui->connectionMessagesWindow->setReadOnly(true);

    m_ui->progressBar->setVisible(false);

    setShowDetailWidgets(false);

    MerSdkManager *sdkManager = MerSdkManager::instance();
    connect(sdkManager, SIGNAL(sdksUpdated()), SLOT(initModel()));

    initModel();
}

MerOptionsWidget::~MerOptionsWidget()
{
    delete m_ui;
    delete m_detailsWidget;
    delete m_model;
    delete m_toolChainUtils;
    delete m_targetUtils;
}

QString MerOptionsWidget::searchKeyWordMatchString() const
{
    const QChar blank = QLatin1Char(' ');
    QString keys;
    int count = m_ui->sdkComboBox->count();
    for (int i = 0; i < count; ++i)
        keys += m_ui->sdkComboBox->itemText(i) + blank;
    if (m_ui->sdkDetailsWidget->isVisible())
        keys += m_detailsWidget->searchKeyWordMatchString();

    return keys.trimmed();
}

void MerOptionsWidget::saveModelData()
{
    saveCurrentData();
}

void MerOptionsWidget::testConnection(bool updateTargets)
{
    m_currentCommand = updateTargets ? UpdateTargets : InstallTargets;

    const QString sdkName = m_ui->sdkComboBox->currentText();
    m_ui->progressBar->setVisible(true);
    if (VirtualBoxManager::isVirtualMachineRunning(sdkName)) {
        MerVirtualMachineManager *remoteManager = MerVirtualMachineManager::instance();
        if (remoteManager->connectToRemote(sdkName, m_detailsWidget->sdk().sshConnectionParams()))
            onConnectionChanged(sdkName, true);
    } else {
        m_ui->progressBar->setVisible(false);
        m_ui->connectionMessagesWindow->appendText(tr("Error: Virtual Machine '%1' "
                                                      "is not running!\n").arg(sdkName));
    }
}

void MerOptionsWidget::showProgress(const QString &message)
{
    m_ui->connectionMessagesWindow->setVisible(true);
    m_ui->connectionMessagesLabel->setVisible(true);
    if (!message.isEmpty())
        m_ui->connectionMessagesWindow->appendText(message);
}

void MerOptionsWidget::onSdkChanged(int currentIndex)
{
    // store the previous sdk details
    if (m_currentSdkIndex != -1) {
        const MerSdk sdk = m_detailsWidget->sdk();
        m_ui->sdkComboBox->setItemData(m_currentSdkIndex, QVariant::fromValue(sdk), SdkRole);
    }
    m_currentSdkIndex = currentIndex;

    if (currentIndex == -1) {
        setShowDetailWidgets(false);
        return;
    }

    // fetch the current sdk details
    const MerSdk sdk = m_ui->sdkComboBox->itemData(currentIndex, SdkRole).value<MerSdk>();
    m_detailsWidget->setSdk(sdk);
    fetchVirtualMachineInfo();
    setShowDetailWidgets(true);
    m_ui->removeButton->setEnabled(!sdk.isAutoDetected());
    // TODO: Remove the restriction of adding only 1 SDK
    m_ui->addButton->setEnabled(false);
}

void MerOptionsWidget::onAddButtonClicked()
{
    SdkSelectionDialog dialog(this);
    dialog.setWindowTitle(tr("Add Mer SDK"));
    if (dialog.exec() != QDialog::Accepted)
        return;
    MerSdk sdk;
    sdk.setVirtualMachineName(dialog.selectedSdkName());
    QStandardItem *rootItem = m_model->invisibleRootItem();
    QStandardItem *item = new QStandardItem;
    item->setData(sdk.virtualMachineName(), Qt::DisplayRole);
    item->setData(QVariant::fromValue(sdk), SdkRole);
    rootItem->appendRow(item);
    m_ui->sdkComboBox->setCurrentIndex(m_ui->sdkComboBox->count() - 1);
    m_ui->messageInfoLabel->setVisible(!rootItem->rowCount());
    // TODO: Remove the restriction of adding only 1 SDK
    m_ui->addButton->setEnabled(false);
}

void MerOptionsWidget::onRemoveButtonClicked()
{
    MerSdk sdk = m_detailsWidget->sdk();
    const QStringList installedTargets;
    m_ui->progressBar->setVisible(true);
    SdkScriptsUtils::removeSdkScripts(sdk);
    m_toolChainUtils->updateToolChains(sdk, installedTargets);
    m_targetUtils->updateQtVersions(sdk, installedTargets);
    SdkKitUtils::updateKits(sdk, installedTargets);
    m_ui->connectionMessagesWindow->appendText(tr("Success: '%1' has been removed \n").arg(
                                                   m_ui->sdkComboBox->currentText()));
    m_ui->progressBar->setVisible(false);
    m_currentSdkIndex = -1;
    m_detailsWidget->setSdk(sdk, false);
    m_ui->sdkComboBox->removeItem(m_ui->sdkComboBox->currentIndex());
    m_ui->messageInfoLabel->setVisible(!m_model->rowCount());
    // TODO: Remove the restriction of adding only 1 SDK
    m_ui->addButton->setEnabled(true);
}

void MerOptionsWidget::onUpdateKitsButtonClicked(const QStringList &targets)
{
    m_targetsToInstall = targets;
    testConnection(false);
}

void MerOptionsWidget::onTestConnectionButtonClicked()
{
    m_ui->connectionMessagesWindow->grayOutOldContent();;
    testConnection();
}

void MerOptionsWidget::onDeployPublicKeyButtonClicked()
{
    m_ui->progressBar->setVisible(false);
    MerSdk sdk = m_detailsWidget->sdk();
    const QString privKeyPath = sdk.sshConnectionParams().privateKeyFile;
    const QString pubKeyPath = privKeyPath + QLatin1String(".pub");
    bool success =
            MerSdkManager::instance()->authorizePublicKey(sdk.virtualMachineName(), pubKeyPath,
                                                          sdk.sshConnectionParams().userName, this);
    if (success)
        m_ui->connectionMessagesWindow->appendText(tr("Success: Deployed public key.\n"));
}

void MerOptionsWidget::onStartVirtualMachineButtonClicked()
{
    MerVirtualMachineManager::instance()->startRemote(m_ui->sdkComboBox->currentText(),
                                                      m_detailsWidget->sdk().sshConnectionParams());
}

void MerOptionsWidget::onLaunchSDKControlCenterClicked()
{
    QDesktopServices::openUrl(QUrl(QLatin1String(CONTROLCENTER_URL)));
}

void MerOptionsWidget::onInstalledTargets(const QString &sdkName, const QStringList &targets)
{
    m_ui->progressBar->setVisible(false);
    if (m_ui->sdkComboBox->currentText() != sdkName)
        return;
    m_detailsWidget->setValidTargets(targets);
}

void MerOptionsWidget::onConnectionError(const QString &sdkName, const QString &error)
{
    showProgress(tr("Error: Cannot connect to Virtual Machine '%1'. %2.\n").arg(sdkName, error));
}

void MerOptionsWidget::onConnectionChanged(const QString &sdkName, bool success)
{
    m_ui->progressBar->setVisible(false);
    if (m_ui->sdkComboBox->currentText() != sdkName)
        return;

    if (!success)
        return;
    else
        showProgress(tr("Success: Can connect to Virtual Machine '%1'.\n").arg(sdkName));

    switch (m_currentCommand) {
    case UpdateTargets:
        updateTargets();
        break;
    case InstallTargets:
        installTargets();
        break;
    case None:
    default:
        break;
    }
    m_currentCommand = None;
}

void MerOptionsWidget::changeState()
{
    m_ui->progressBar->setVisible(true);
    switch (m_state) {
    case SaveCurrentData: {
        m_tempSdk = m_detailsWidget->sdk();
        saveCurrentData();
        m_state = CreateNewScripts;
        m_timer.start();
        break;
    }
    case CreateNewScripts: {
        SdkScriptsUtils::createNewScripts(m_tempSdk, m_targetsToInstall);
        m_state = UpdateToolChains;
        m_timer.start();
        break;
    }
    case UpdateToolChains: {
        m_toolChainUtils->updateToolChains(m_tempSdk, m_targetsToInstall);
        m_state = UpdateQtVersions;
        m_timer.start();
        break;
    }
    case UpdateQtVersions: {
        m_targetUtils->updateQtVersions(m_tempSdk, m_targetsToInstall);
        m_state = UpdateKits;
        m_timer.start();
        break;
    }
    case UpdateKits: {
        SdkKitUtils::updateKits(m_tempSdk, m_targetsToInstall);
        m_state = RemoveInvalidScripts;
        m_timer.start();
        break;
    }
    case RemoveInvalidScripts: {
        SdkScriptsUtils::removeInvalidScripts(m_tempSdk, m_targetsToInstall);
        m_state = Done;
        m_timer.start();
        break;
    }
    case Done: {
        m_detailsWidget->setSdk(m_tempSdk);
        m_state = Idle;
        m_ui->progressBar->setVisible(false);
        m_ui->connectionMessagesWindow->appendText(tr("Success: Targets have been updated.\n"));
        updateTargets();
        break;
    }
    case Idle:
    default:
        m_ui->progressBar->setVisible(false);
        break;
    }
}

void MerOptionsWidget::onCreateSshKey(const QString &privKeyPath)
{
    if (QFileInfo(privKeyPath).exists()) {
        m_ui->connectionMessagesWindow->appendText(tr("Error: File '%1' exists.\n").arg(
                                                       privKeyPath));
        return;
    }

    bool success = true;
    QSsh::SshKeyGenerator keyGen;
    success = keyGen.generateKeys(QSsh::SshKeyGenerator::Rsa,
                                  QSsh::SshKeyGenerator::Mixed, 2048,
                                  QSsh::SshKeyGenerator::DoNotOfferEncryption);
    if (!success) {
        m_ui->connectionMessagesWindow->appendText(tr("Error: %1\n").arg(keyGen.error()));
        return;
    }

    Utils::FileSaver privKeySaver(privKeyPath);
    privKeySaver.write(keyGen.privateKey());
    success = privKeySaver.finalize();
    if (!success) {
        m_ui->connectionMessagesWindow->appendText(tr("Error: %1\n").arg(
                                                       privKeySaver.errorString()));
        return;
    }

    Utils::FileSaver pubKeySaver(privKeyPath + QLatin1String(".pub"));
    const QByteArray publicKey = keyGen.publicKey();
    pubKeySaver.write(publicKey);
    success = pubKeySaver.finalize();
    if (!success) {
        m_ui->connectionMessagesWindow->appendText(tr("Error: %1\n").arg(
                                                       pubKeySaver.errorString()));
        return;
    }

    m_ui->connectionMessagesWindow->appendText(tr("Success: SSH keys created.\n"));
}

void MerOptionsWidget::initModel()
{
    if (m_model)
        delete m_model;
    m_model = new QStandardItemModel();

    MerSdkManager *sdkManager = MerSdkManager::instance();
    QList<MerSdk> sdks = sdkManager->sdks();

    QStandardItem *rootItem = m_model->invisibleRootItem();
    foreach (const MerSdk &sdk, sdks) {
        QStandardItem *item = new QStandardItem();
        item->setData(sdk.virtualMachineName(), Qt::DisplayRole);
        item->setData(QVariant::fromValue(sdk), SdkRole);
        rootItem->appendRow(item);
    }
    m_ui->sdkComboBox->setModel(m_model);
    m_ui->messageInfoLabel->setVisible(!rootItem->rowCount());
}

void MerOptionsWidget::setShowDetailWidgets(bool show)
{
    m_ui->sdkDetailsWidget->setVisible(show);
    m_ui->removeButton->setEnabled(show);
    m_ui->startVirtualMachineButton->setEnabled(show);
    m_ui->launchSDKControlCenterPushButton->setEnabled(show);
}

void MerOptionsWidget::fetchVirtualMachineInfo()
{
    m_ui->progressBar->setVisible(true);
    VirtualMachineInfo info =
            VirtualBoxManager::fetchVirtualMachineInfo(m_ui->sdkComboBox->currentText());
    MerSdk sdk = m_detailsWidget->sdk();
    QSsh::SshConnectionParameters params = sdk.sshConnectionParams();
    if (info.sshPort)
        params.port = info.sshPort;
    sdk.setSshConnectionParameters(params);
    if (!info.sharedHome.isEmpty())
        sdk.setSharedHomePath(info.sharedHome);
    if (!info.sharedTarget.isEmpty())
        sdk.setSharedTargetPath(info.sharedTarget);
    if (!info.sharedSsh.isEmpty())
        sdk.setSharedSshPath(info.sharedSsh);
    m_detailsWidget->setSdk(sdk);
    m_ui->progressBar->setVisible(false);
    // Test connection if we have the correct params
    bool test = params.authenticationType == QSsh::SshConnectionParameters::AuthenticationByPassword
            ? !params.password.isEmpty() : !params.privateKeyFile.isEmpty();
    if (test)
        testConnection();
}

void MerOptionsWidget::updateTargets()
{
    m_targetUtils->setSshConnectionParameters(m_detailsWidget->sdk().sshConnectionParams());
    m_ui->progressBar->setVisible(true);
    if (!m_targetUtils->checkInstalledTargets(m_ui->sdkComboBox->currentText())) {
        m_ui->progressBar->setVisible(false);
        m_ui->connectionMessagesWindow->appendText(tr("Error: Could not update targets."
                                                      "An instance of the process is already "
                                                      "running. Please try again.\n"));
    }
}

void MerOptionsWidget::installTargets()
{
    m_state = SaveCurrentData;
    m_timer.start();
}

void MerOptionsWidget::saveCurrentData()
{
    MerSdkManager *sdkManager = MerSdkManager::instance();
    QString sdkName = m_ui->sdkComboBox->itemData(m_currentSdkIndex, Qt::DisplayRole).toString();
    if (sdkManager->contains(sdkName))
        sdkManager->removeSdk(sdkName);
    if (!sdkName.isEmpty() && m_detailsWidget->isSdkValid())
        sdkManager->addSdk(sdkName, m_detailsWidget->sdk());
    sdkManager->writeSettings();
}

} // Internal
} // Mer
