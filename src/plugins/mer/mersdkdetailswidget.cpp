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

#include "mersdkdetailswidget.h"
#include "ui_mersdkdetailswidget.h"

#include "merconstants.h"
#include "mersdkmanager.h"
#include "mervirtualboxmanager.h"

#include <utils/hostosinfo.h>

#include <QFileDialog>
#include <QMessageBox>

using namespace Utils;

namespace Mer {
namespace Internal {

MerSdkDetailsWidget::MerSdkDetailsWidget(QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui::MerSdkDetailsWidget)
    , m_invalidIcon(QLatin1String(":/mer/images/compile_error.png"))
    , m_warningIcon(QLatin1String(":/mer/images/warning.png"))
    , m_updateConnection(false)
{
    m_ui->setupUi(this);

    connect(m_ui->authorizeSshKeyPushButton, &QPushButton::clicked,
            this, &MerSdkDetailsWidget::onAuthorizeSshKeyButtonClicked);
    connect(m_ui->generateSshKeyPushButton, &QPushButton::clicked,
            this, &MerSdkDetailsWidget::onGenerateSshKeyButtonClicked);
    connect(m_ui->privateKeyPathChooser, &PathChooser::editingFinished,
            this, &MerSdkDetailsWidget::onPathChooserEditingFinished);
    connect(m_ui->privateKeyPathChooser, &PathChooser::browsingFinished,
            this, &MerSdkDetailsWidget::onPathChooserEditingFinished);
    connect(m_ui->testConnectionPushButton, &QPushButton::clicked,
            this, &MerSdkDetailsWidget::testConnectionButtonClicked);
    connect(m_ui->sshTimeoutSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &MerSdkDetailsWidget::sshTimeoutChanged);
    connect(m_ui->headlessCheckBox, &QCheckBox::toggled,
            this, &MerSdkDetailsWidget::headlessCheckBoxToggled);
    connect(m_ui->srcFolderApplyButton, &QPushButton::clicked,
            this, &MerSdkDetailsWidget::onSrcFolderApplyButtonClicked);

    m_ui->privateKeyPathChooser->setExpectedKind(PathChooser::File);
    m_ui->privateKeyPathChooser->setPromptDialogTitle(tr("Select SSH Key"));
}

MerSdkDetailsWidget::~MerSdkDetailsWidget()
{
    delete m_ui;
}

QString MerSdkDetailsWidget::searchKeyWordMatchString() const
{
    const QChar blank = QLatin1Char(' ');
    return  m_ui->privateKeyPathChooser->path() + blank
            + m_ui->homeFolderPathLabel->text() + blank
            + m_ui->targetFolderPathLabel->text() + blank
            + m_ui->sshFolderPathLabel->text();
}

void MerSdkDetailsWidget::setSrcFolderChooserPath(const QString& path)
{
    m_ui->srcFolderPathChooser->setPath(QDir::toNativeSeparators(path));
}

void MerSdkDetailsWidget::setSdk(const MerSdk *sdk)
{
    m_ui->nameLabelText->setText(sdk->virtualMachineName());
    m_ui->sshPortLabelText->setText(QString::number(sdk->sshPort()));
    m_ui->wwwPortLabelText->setText(QString::number(sdk->wwwPort()));
    m_ui->homeFolderPathLabel->setText(QDir::toNativeSeparators(sdk->sharedHomePath()));
    m_ui->targetFolderPathLabel->setText(QDir::toNativeSeparators(sdk->sharedTargetsPath()));
    m_ui->sshFolderPathLabel->setText(QDir::toNativeSeparators(sdk->sharedSshPath()));
    m_ui->configFolderPathLabel->setText(QDir::toNativeSeparators(sdk->sharedConfigPath()));
    m_ui->srcFolderPathChooser->setPath(QDir::toNativeSeparators(sdk->sharedSrcPath()));

    if (MerSdkManager::hasSdk(sdk)) {
        const QStringList &targets = sdk->targetNames();
        if (targets.isEmpty())
            m_ui->targetsListLabel->setText(tr("No targets installed"));
        else
            m_ui->targetsListLabel->setText(sdk->targetNames().join(QLatin1String(", ")));
    } else {
        m_ui->targetsListLabel->setText(tr("Add SDK first to see targets"));
    }

    if (!sdk->sharedSshPath().isEmpty()) {
        const QString authorized_keys = QDir::fromNativeSeparators(sdk->sharedSshPath());
        m_ui->authorizeSshKeyPushButton->setToolTip(tr("Add public key to %1").arg(
                                                        QDir::toNativeSeparators(authorized_keys)));
    }

    m_ui->userNameLabelText->setText(sdk->userName());
}

void MerSdkDetailsWidget::setTestButtonEnabled(bool enabled)
{
    m_ui->testConnectionPushButton->setEnabled(enabled);
}

void MerSdkDetailsWidget::setPrivateKeyFile(const QString &path)
{
    m_ui->privateKeyPathChooser->setPath(path);
    m_ui->privateKeyPathChooser->triggerChanged();
    onPathChooserEditingFinished();
}

void MerSdkDetailsWidget::setStatus(const QString &status)
{
    m_ui->statusLabelText->setText(status);
}

void MerSdkDetailsWidget::setSshTimeout(int timeout)
{
    m_ui->sshTimeoutSpinBox->setValue(timeout);
}

void MerSdkDetailsWidget::setHeadless(bool enabled)
{
    m_ui->headlessCheckBox->setChecked(enabled);
}

void MerSdkDetailsWidget::onSrcFolderApplyButtonClicked()
{
    if (m_ui->srcFolderPathChooser->isValid()) {
        QString path = m_ui->srcFolderPathChooser->path();
        if (HostOsInfo::isWindowsHost()) {
            if (!path.endsWith(QLatin1Char('\\')) && !path.endsWith(QLatin1Char('/'))) {
                path = path + QLatin1Char('\\');
            }
        }
        emit srcFolderApplyButtonClicked(path);
    } else {
        QMessageBox::warning(this, tr("Invalid path"),
                tr("Not a valid source folder path: %1").arg(m_ui->srcFolderPathChooser->path()));
    }
}

void MerSdkDetailsWidget::onAuthorizeSshKeyButtonClicked()
{
    if (m_ui->privateKeyPathChooser->isValid())
        emit authorizeSshKey(m_ui->privateKeyPathChooser->path());
}

void MerSdkDetailsWidget::onGenerateSshKeyButtonClicked()
{
    emit generateSshKey(m_ui->privateKeyPathChooser->path());
}

void MerSdkDetailsWidget::onPathChooserEditingFinished()
{
    if (m_ui->privateKeyPathChooser->isValid())
        emit sshKeyChanged(m_ui->privateKeyPathChooser->path());
}

} // Internal
} // Mer
