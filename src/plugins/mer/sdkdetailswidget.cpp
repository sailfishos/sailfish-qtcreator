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

#include "sdkdetailswidget.h"
#include "ui_sdkdetailswidget.h"
#include "merconstants.h"
#include "sdkkitutils.h"
#include "mersdkmanager.h"

#include <QDesktopServices>
#include <QFileDialog>

namespace Mer {
namespace Internal {

SdkDetailsWidget::SdkDetailsWidget(QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui::SdkDetailsWidget)
    , m_invalidIcon(QLatin1String(":/mer/images/compile_error.png"))
    , m_warningIcon(QLatin1String(":/mer/images/warning.png"))
    , m_updateConnection(false)
    , m_validSdk(false)
{
    m_ui->setupUi(this);

    QButtonGroup *buttonGroup = new QButtonGroup(this);
    buttonGroup->setExclusive(true);
    buttonGroup->addButton(m_ui->createSshKeyRadioButton);
    buttonGroup->addButton(m_ui->useExistingSshKeyRadioButton);
    connect(buttonGroup, SIGNAL(buttonClicked(int)), SLOT(onSshKeyRadioButton()));

    connect(m_ui->createSshKeySaveToolButton, SIGNAL(clicked()), SLOT(onSshKeySaveButtonClicked()));
    connect(m_ui->createSshKeyPushButton, SIGNAL(clicked()), SLOT(onSshKeyButtonClicked()));

    connect(m_ui->createSshKeyLineEdit, SIGNAL(editingFinished()),
            SLOT(onAuthenticationInfoUpdated()));
    connect(m_ui->createSshKeyLineEdit, SIGNAL(textChanged(QString)),
            SLOT(onAuthenticationInfoChanged()));
    connect(m_ui->privateKeyPathChooser, SIGNAL(editingFinished()),
            SLOT(onAuthenticationInfoUpdated()));
    connect(m_ui->privateKeyPathChooser, SIGNAL(changed(QString)),
            SLOT(onAuthenticationInfoChanged()));

    connect(m_ui->testConnectionButton, SIGNAL(clicked()), SLOT(onTestConnectionButtonClicked()));
    connect(m_ui->deployPublicKeyButton, SIGNAL(clicked()),
            SLOT(onAuthorizePublicKeyButtonClicked()));
    connect(m_ui->updateKitsButton, SIGNAL(clicked()), SLOT(onUpdateKitsButtonClicked()));

    connect(m_ui->targetsListWidget, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
            SLOT(onCurrentItemChanged(QListWidgetItem*)));

    connect(m_ui->createKitButton, SIGNAL(clicked()), SLOT(onCreateKitButtonClicked()));
    connect(m_ui->removeKitButton, SIGNAL(clicked()), SLOT(onRemoveKitButtonClicked()));

    m_ui->privateKeyPathChooser->setExpectedKind(Utils::PathChooser::File);
    const QString sshDirectory(QDir::fromNativeSeparators(QDesktopServices::storageLocation(
                                                              QDesktopServices::HomeLocation))
                               + QLatin1String("/.ssh"));
    m_ui->privateKeyPathChooser->setBaseDirectory(sshDirectory);
    m_ui->privateKeyPathChooser->setPromptDialogTitle(tr("Select SSH Key"));

    m_ui->createSshKeyLineEdit->setText(
                QDir::toNativeSeparators(sshDirectory + QLatin1String("/mer-qt-creator-rsa")));
    m_ui->privateKeyPathChooser->setPath(QDir::toNativeSeparators(
                                             QString::fromLatin1("%1/id_rsa").arg(sshDirectory)));

    m_ui->updateKitsButton->setEnabled(false);
    m_ui->createKitButton->setEnabled(false);
    m_ui->removeKitButton->setEnabled(false);
    m_ui->useExistingSshKeyRadioButton->setChecked(true);
    m_ui->testConnectionButton->setEnabled(false);
    m_ui->deployPublicKeyButton->setEnabled(false);

    onSshKeyRadioButton();
}

SdkDetailsWidget::~SdkDetailsWidget()
{
    delete m_ui;
}

QString SdkDetailsWidget::searchKeyWordMatchString() const
{
    const QChar blank = QLatin1Char(' ');
    return  m_ui->createSshKeyLineEdit->text() + blank
            + m_ui->privateKeyPathChooser->path() + blank
            + m_ui->homeFolderPathLabel->text() + blank
            + m_ui->targetFolderPathLabel->text() + blank
            + m_ui->sshFolderPathLabel->text();
}

bool SdkDetailsWidget::isSdkValid() const
{
    return m_validSdk;
}

void SdkDetailsWidget::setSdk(const MerSdk &sdk, bool validSdk)
{
    m_validSdk = validSdk;
    m_sdk = sdk;
    QSsh::SshConnectionParameters params = m_sdk.sshConnectionParams();
    const bool useExistingKey = !params.privateKeyFile.isEmpty();
    m_ui->privateKeyPathChooser->setPath(params.privateKeyFile);
    m_ui->createSshKeyRadioButton->setChecked(!useExistingKey);
    m_ui->useExistingSshKeyRadioButton->setChecked(useExistingKey);
    onSshKeyRadioButton();
    const QString homePath = sdk.sharedHomePath();
    m_ui->homeFolderPathLabel->setText(QDir::toNativeSeparators(homePath));
    m_ui->targetFolderPathLabel->setText(QDir::toNativeSeparators(sdk.sharedTargetPath()));
    m_ui->sshFolderPathLabel->setText(QDir::toNativeSeparators(sdk.sharedSshPath()));
    bool isValidSdk = !homePath.isEmpty();
    m_ui->testConnectionButton->setEnabled(isValidSdk);
    if (isValidSdk) {
        const QString authorized_keys = QDir::fromNativeSeparators(sdk.sharedSshPath())
                + QLatin1String("/.ssh/authorized_keys");
        m_ui->deployPublicKeyButton->setToolTip(tr("Add public key to %1").arg(
                                                    QDir::toNativeSeparators(authorized_keys)));
    }
    setValidTargets(sdk.qtVersions().keys());

    // For Mer VMs installed by the SDK installer, further key modifications
    // should be disabled.
    const bool canModifyKeys = !sdk.isAutoDetected();
    if (m_ui->createSshKeyRadioButton->isChecked()) {
        m_ui->createSshKeyLineEdit->setEnabled(canModifyKeys);
        m_ui->createSshKeySaveToolButton->setEnabled(canModifyKeys);
        m_ui->createSshKeyPushButton->setEnabled(canModifyKeys);
    } else {
        m_ui->privateKeyPathChooser->setEnabled(canModifyKeys);
    }
    m_ui->createSshKeyRadioButton->setEnabled(canModifyKeys);
    m_ui->useExistingSshKeyRadioButton->setEnabled(canModifyKeys);
    m_ui->deployPublicKeyButton->setEnabled(isValidSdk && canModifyKeys);
}

MerSdk SdkDetailsWidget::sdk() const
{
    MerSdk sdk = m_sdk;
    QSsh::SshConnectionParameters params = sdk.sshConnectionParams();
    params.privateKeyFile = m_ui->createSshKeyRadioButton->isChecked()
            ? m_ui->createSshKeyLineEdit->text()
            : m_ui->privateKeyPathChooser->path();
    sdk.setSshConnectionParameters(params);
    sdk.setSharedHomePath(m_ui->homeFolderPathLabel->text());
    sdk.setSharedTargetPath(m_ui->targetFolderPathLabel->text());
    sdk.setSharedSshPath(m_ui->sshFolderPathLabel->text());
    return sdk;
}

void SdkDetailsWidget::setValidTargets(const QStringList &targets)
{
    bool needsUpdateTargets = false;
    m_ui->targetsListWidget->clear();

    QListWidgetItem *item = 0;
    foreach (const QString &t, targets) {
        if (MerSdkManager::validateTarget(m_sdk, t)) {
            item = new QListWidgetItem(t, m_ui->targetsListWidget);
            item->setData(InstallRole, true);
        } else {
            item = new QListWidgetItem(m_warningIcon, t, m_ui->targetsListWidget);
            // if the target has been added by sdk installer then it cannot be updated
            if (!needsUpdateTargets && !SdkKitUtils::isSdkKit(m_sdk.virtualMachineName(), t))
                needsUpdateTargets = true;
            item->setData(InstallRole, false);
        }
        item->setData(ValidRole, true);
    }
    m_ui->updateKitsButton->setEnabled(needsUpdateTargets);
}

QStringList SdkDetailsWidget::validTargets() const
{
    QStringList targets;
    int count = m_ui->targetsListWidget->count();
    for (int i = 0; i < count; ++i) {
        QListWidgetItem *item = m_ui->targetsListWidget->item(i);
        if (item->data(ValidRole).toBool())
            targets << item->text();
    }
    return targets;
}

QStringList SdkDetailsWidget::installedTargets() const
{
    QStringList targets;
    int count = m_ui->targetsListWidget->count();
    for (int i = 0; i < count; ++i) {
        QListWidgetItem *item = m_ui->targetsListWidget->item(i);
        if (item->data(InstallRole).toBool())
            targets << item->text();
    }
    return targets;
}

void SdkDetailsWidget::onSshKeyRadioButton()
{
    const bool existingSshKey = m_ui->useExistingSshKeyRadioButton->isChecked();
    m_ui->createSshKeyLineEdit->setEnabled(!existingSshKey);
    m_ui->createSshKeySaveToolButton->setEnabled(!existingSshKey);
    m_ui->createSshKeyPushButton->setEnabled(!existingSshKey);
    m_ui->privateKeyPathChooser->setEnabled(existingSshKey);
}

void SdkDetailsWidget::onSshKeySaveButtonClicked()
{
    const QString hostSshDirectoryPath =
            QDesktopServices::storageLocation(QDesktopServices::HomeLocation)
            + QLatin1String("/.ssh");
    const QString fileName = QFileDialog::getSaveFileName(this, tr("Save SSH private key"),
                                                          hostSshDirectoryPath);
    m_ui->createSshKeyLineEdit->setText(QDir::toNativeSeparators(fileName));
}

void SdkDetailsWidget::onSshKeyButtonClicked()
{
    emit createSshKey(m_ui->createSshKeyLineEdit->text());
}

void SdkDetailsWidget::onAuthenticationInfoChanged()
{
    const bool authorizeKey = m_ui->createSshKeyRadioButton->isChecked()
            || (m_ui->useExistingSshKeyRadioButton->isChecked()
                && m_ui->privateKeyPathChooser->isValid());
    m_ui->deployPublicKeyButton->setEnabled(authorizeKey);
}

void SdkDetailsWidget::onAuthenticationInfoUpdated()
{
    if (m_updateConnection)
        emit testConnection();
    m_updateConnection = false;
}

void SdkDetailsWidget::onCurrentItemChanged(QListWidgetItem *item)
{
    if (!item) {
        m_ui->createKitButton->setEnabled(false);
        m_ui->removeKitButton->setEnabled(false);
        return;
    }
    const bool isInstalled = item->data(InstallRole).toBool();
    const bool isSdkKit = SdkKitUtils::isSdkKit(m_sdk.virtualMachineName(), item->text());
    m_ui->createKitButton->setEnabled(!isInstalled && !isSdkKit);
    m_ui->removeKitButton->setEnabled(isInstalled && !isSdkKit);
}

void SdkDetailsWidget::onUpdateKitsButtonClicked()
{
    emit updateKits(validTargets());
}

void SdkDetailsWidget::onCreateKitButtonClicked()
{
    QStringList targets = installedTargets();
    targets << m_ui->targetsListWidget->currentItem()->text();
    emit updateKits(targets);
}

void SdkDetailsWidget::onRemoveKitButtonClicked()
{
    QStringList targets = installedTargets();
    targets.removeOne(m_ui->targetsListWidget->currentItem()->text());
    emit updateKits(targets);
}

void SdkDetailsWidget::onTestConnectionButtonClicked()
{
    emit testConnection();
}

void SdkDetailsWidget::onAuthorizePublicKeyButtonClicked()
{
    emit deployPublicKey();
}

} // Internal
} // Mer
