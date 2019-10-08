/****************************************************************************
**
** Copyright (C) 2012-2015,2017-2019 Jolla Ltd.
** Copyright (C) 2019 Open Mobile Platform LLC.
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

#include "merbuildenginedetailswidget.h"
#include "ui_merbuildenginedetailswidget.h"

#include "merconstants.h"
#include "mersdkmanager.h"

#include <sfdk/buildengine.h>
#include <sfdk/sdk.h>
#include <sfdk/virtualmachine.h>

#include <ssh/sshconnection.h>
#include <utils/algorithm.h>
#include <utils/hostosinfo.h>
#include <utils/utilsicons.h>

#include <QFileDialog>
#include <QMessageBox>

using namespace Utils;
using namespace Sfdk;
using namespace Mer::Constants;

namespace Mer {
namespace Internal {

MerBuildEngineDetailsWidget::MerBuildEngineDetailsWidget(QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui::MerBuildEngineDetailsWidget)
{
    m_ui->setupUi(this);

    m_ui->sshPortInfoLabel->setPixmap(Utils::Icons::INFO.pixmap());
    m_ui->sshPortInfoLabel->setToolTip(
            QLatin1String("<font color=\"red\">")
            + tr("Stop the virtual machine to unlock this field for editing.")
            + QLatin1String("</font>"));

    m_ui->wwwPortInfoLabel->setPixmap(Utils::Icons::INFO.pixmap());
    m_ui->wwwPortInfoLabel->setToolTip(
            QLatin1String("<font color=\"red\">")
            + tr("Stop the virtual machine to unlock this field for editing.")
            + QLatin1String("</font>"));

    connect(m_ui->authorizeSshKeyPushButton, &QPushButton::clicked,
            this, &MerBuildEngineDetailsWidget::onAuthorizeSshKeyButtonClicked);
    connect(m_ui->generateSshKeyPushButton, &QPushButton::clicked,
            this, &MerBuildEngineDetailsWidget::onGenerateSshKeyButtonClicked);
    connect(m_ui->privateKeyPathChooser, &PathChooser::editingFinished,
            this, &MerBuildEngineDetailsWidget::onPathChooserEditingFinished);
    connect(m_ui->privateKeyPathChooser, &PathChooser::browsingFinished,
            this, &MerBuildEngineDetailsWidget::onPathChooserEditingFinished);
    connect(m_ui->testConnectionPushButton, &QPushButton::clicked,
            this, &MerBuildEngineDetailsWidget::testConnectionButtonClicked);
    connect(m_ui->sshPortSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &MerBuildEngineDetailsWidget::sshPortChanged);
    connect(m_ui->sshTimeoutSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &MerBuildEngineDetailsWidget::sshTimeoutChanged);
    connect(m_ui->headlessCheckBox, &QCheckBox::toggled,
            this, &MerBuildEngineDetailsWidget::headlessCheckBoxToggled);
    connect(m_ui->srcFolderApplyButton, &QPushButton::clicked,
            this, &MerBuildEngineDetailsWidget::onSrcFolderApplyButtonClicked);
    connect(m_ui->wwwPortSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &MerBuildEngineDetailsWidget::wwwPortChanged);
    connect(m_ui->virtualMachineSettingsWidget, &MerVirtualMachineSettingsWidget::memorySizeMbChanged,
            this, &MerBuildEngineDetailsWidget::memorySizeMbChanged);
    connect(m_ui->virtualMachineSettingsWidget, &MerVirtualMachineSettingsWidget::cpuCountChanged,
            this, &MerBuildEngineDetailsWidget::cpuCountChanged);
    connect(m_ui->virtualMachineSettingsWidget, &MerVirtualMachineSettingsWidget::vdiCapacityMbChnaged,
            this, &MerBuildEngineDetailsWidget::vdiCapacityMbChnaged);
    connect(m_ui->wwwProxyDisabledButton, &QRadioButton::toggled,
            this, &MerBuildEngineDetailsWidget::onWwwProxyDisabledToggled);
    connect(m_ui->wwwProxyAutomaticButton, &QRadioButton::toggled,
            this, &MerBuildEngineDetailsWidget::onWwwProxyAutomaticToggled);
    connect(m_ui->wwwProxyManualButton, &QRadioButton::toggled,
            this, &MerBuildEngineDetailsWidget::onWwwProxyManualToggled);
    connect(m_ui->wwwProxyServersLine, &QLineEdit::textEdited,
            this, &MerBuildEngineDetailsWidget::onWwwProxyServersEdited);
    connect(m_ui->wwwProxyExcludesLine, &QLineEdit::textEdited,
            this, &MerBuildEngineDetailsWidget::onWwwProxyExcludesEdited);

    m_ui->privateKeyPathChooser->setExpectedKind(PathChooser::File);
    m_ui->privateKeyPathChooser->setPromptDialogTitle(tr("Select SSH Key"));
}

MerBuildEngineDetailsWidget::~MerBuildEngineDetailsWidget()
{
    delete m_ui;
}

QString MerBuildEngineDetailsWidget::searchKeyWordMatchString() const
{
    const QChar blank = QLatin1Char(' ');
    return  m_ui->privateKeyPathChooser->path() + blank
            + m_ui->homeFolderPathLabel->text() + blank
            + m_ui->targetFolderPathLabel->text() + blank
            + m_ui->sshFolderPathLabel->text();
}

void MerBuildEngineDetailsWidget::setSrcFolderChooserPath(const QString& path)
{
    m_ui->srcFolderPathChooser->setPath(QDir::toNativeSeparators(path));
}

void MerBuildEngineDetailsWidget::setBuildEngine(const Sfdk::BuildEngine *buildEngine)
{
    m_ui->nameLabelText->setText(buildEngine->name());
    m_ui->autodetectedLabelText->setText(buildEngine->isAutodetected() ? tr("Yes") : tr("No"));
    m_ui->homeFolderPathLabel->setText(QDir::toNativeSeparators(buildEngine->sharedHomePath().toString()));
    m_ui->targetFolderPathLabel->setText(QDir::toNativeSeparators(buildEngine->sharedTargetsPath().toString()));
    m_ui->sshFolderPathLabel->setText(QDir::toNativeSeparators(buildEngine->sharedSshPath().toString()));
    m_ui->configFolderPathLabel->setText(QDir::toNativeSeparators(buildEngine->sharedConfigPath().toString()));
    m_ui->srcFolderPathChooser->setPath(QDir::toNativeSeparators(buildEngine->sharedSrcPath().toString()));

    if (Sdk::buildEngines().contains(const_cast<BuildEngine *>(buildEngine))) {
        const QStringList targets = buildEngine->buildTargetNames();
        if (targets.isEmpty())
            m_ui->targetsListLabel->setText(tr("No build targets installed"));
        else
            m_ui->targetsListLabel->setText(targets.join(QLatin1String(", ")));
    } else {
        m_ui->targetsListLabel->setText(tr("Complete adding the build engine to see its build targets"));
    }

    if (!buildEngine->sharedSshPath().isEmpty()) {
        const QString authorized_keys = QDir::fromNativeSeparators(buildEngine->sharedSshPath().toString());
        m_ui->authorizeSshKeyPushButton->setToolTip(tr("Add public key to %1").arg(
                                                        QDir::toNativeSeparators(authorized_keys)));
    }

    m_ui->userNameLabelText->setText(buildEngine->virtualMachine()->sshParameters().userName());

    m_ui->virtualMachineSettingsWidget->setMemorySizeMb(buildEngine->virtualMachine()->memorySizeMb());
    m_ui->virtualMachineSettingsWidget->setCpuCount(buildEngine->virtualMachine()->cpuCount());
    m_ui->virtualMachineSettingsWidget->setVdiCapacityMb(buildEngine->virtualMachine()->vdiCapacityMb());
}

void MerBuildEngineDetailsWidget::setTestButtonEnabled(bool enabled)
{
    m_ui->testConnectionPushButton->setEnabled(enabled);
}

void MerBuildEngineDetailsWidget::setPrivateKeyFile(const QString &path)
{
    m_ui->privateKeyPathChooser->setPath(path);
    m_ui->privateKeyPathChooser->triggerChanged();
    onPathChooserEditingFinished();
}

void MerBuildEngineDetailsWidget::setStatus(const QString &status)
{
    m_ui->statusLabelText->setText(status);
}

void MerBuildEngineDetailsWidget::setVmOffStatus(bool vmOff)
{
    m_ui->sshPortSpinBox->setEnabled(vmOff);
    m_ui->sshPortInfoLabel->setVisible(!vmOff);
    m_ui->wwwPortSpinBox->setEnabled(vmOff);
    m_ui->wwwPortInfoLabel->setVisible(!vmOff);
    m_ui->virtualMachineSettingsWidget->setVmOff(vmOff);
}

void MerBuildEngineDetailsWidget::setSshTimeout(int timeout)
{
    m_ui->sshTimeoutSpinBox->setValue(timeout);
}

void MerBuildEngineDetailsWidget::setSshPort(quint16 port)
{
    m_ui->sshPortSpinBox->setValue(port);
}

void MerBuildEngineDetailsWidget::setHeadless(bool enabled)
{
    m_ui->headlessCheckBox->setChecked(enabled);
}

void MerBuildEngineDetailsWidget::setWwwPort(quint16 port)
{
    m_ui->wwwPortSpinBox->setValue(port);
}

void MerBuildEngineDetailsWidget::setMemorySizeMb(int sizeMb)
{
    m_ui->virtualMachineSettingsWidget->setMemorySizeMb(sizeMb);
}

void MerBuildEngineDetailsWidget::setCpuCount(int count)
{
    m_ui->virtualMachineSettingsWidget->setCpuCount(count);
}

void MerBuildEngineDetailsWidget::setVdiCapacityMb(int capacityMb)
{
    m_ui->virtualMachineSettingsWidget->setVdiCapacityMb(capacityMb);
}

void MerBuildEngineDetailsWidget::setWwwProxy(const QString &type, const QString &servers, const QString &excludes)
{
    if (type == MER_SDK_PROXY_AUTOMATIC) {
        m_wwwProxyServerUrl = servers;
        m_ui->wwwProxyAutomaticButton->click();
    } else if (type == MER_SDK_PROXY_MANUAL) {
        m_wwwProxyServerList = servers;
        m_wwwProxyExcludes = excludes;
        m_ui->wwwProxyManualButton->click();
    } else {
        m_ui->wwwProxyDisabledButton->click();
    }
}

void MerBuildEngineDetailsWidget::onSrcFolderApplyButtonClicked()
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

void MerBuildEngineDetailsWidget::onAuthorizeSshKeyButtonClicked()
{
    if (m_ui->privateKeyPathChooser->isValid())
        emit authorizeSshKey(m_ui->privateKeyPathChooser->path());
}

void MerBuildEngineDetailsWidget::onGenerateSshKeyButtonClicked()
{
    emit generateSshKey(m_ui->privateKeyPathChooser->path());
}

void MerBuildEngineDetailsWidget::onPathChooserEditingFinished()
{
    if (m_ui->privateKeyPathChooser->isValid())
        emit sshKeyChanged(m_ui->privateKeyPathChooser->path());
}

void MerBuildEngineDetailsWidget::onWwwProxyDisabledToggled(bool checked)
{
    if (!checked)
        return;

    m_wwwProxy = MER_SDK_PROXY_DISABLED;

    m_ui->wwwProxyServersLabel->setHidden(true);

    m_ui->wwwProxyServersLine->clear();
    m_ui->wwwProxyServersLine->setHidden(true);

    m_ui->wwwProxyExcludesLabel->setHidden(true);

    m_ui->wwwProxyExcludesLine->clear();
    m_ui->wwwProxyExcludesLine->setHidden(true);

    emit wwwProxyChanged(m_wwwProxy, QString(), QString());
}

void MerBuildEngineDetailsWidget::onWwwProxyAutomaticToggled(bool checked)
{
    if (!checked || m_wwwProxy == MER_SDK_PROXY_AUTOMATIC)
        return;

    m_wwwProxy = MER_SDK_PROXY_AUTOMATIC;

    m_ui->wwwProxyServersLabel->setText(tr("URL:"));
    m_ui->wwwProxyServersLabel->setHidden(false);

    m_ui->wwwProxyServersLine->setText(m_wwwProxyServerUrl);
    m_ui->wwwProxyServersLine->setHidden(false);
    m_ui->wwwProxyServersLine->setToolTip(tr("Leave empty for asking the network, otherwise insert a proxy autoconfiguration URL."));

    m_ui->wwwProxyExcludesLabel->setHidden(true);
    m_ui->wwwProxyExcludesLine->clear();
    m_ui->wwwProxyExcludesLine->setHidden(true);

    emit wwwProxyChanged(m_wwwProxy, m_wwwProxyServerUrl, QString());
}

void MerBuildEngineDetailsWidget::onWwwProxyManualToggled(bool checked)
{
    if (!checked || m_wwwProxy == MER_SDK_PROXY_MANUAL)
        return;

    m_wwwProxy = MER_SDK_PROXY_MANUAL;

    m_ui->wwwProxyServersLabel->setText(tr("Servers:"));
    m_ui->wwwProxyServersLabel->setHidden(false);

    m_ui->wwwProxyServersLine->setText(m_wwwProxyServerList);
    m_ui->wwwProxyServersLine->setHidden(false);
    m_ui->wwwProxyServersLine->setToolTip(tr("Insert a list of proxies. E.g. \"proxy1.example.com 1.2.3.4:8080\""));

    m_ui->wwwProxyExcludesLabel->setHidden(false);

    m_ui->wwwProxyExcludesLine->setText(m_wwwProxyExcludes);
    m_ui->wwwProxyExcludesLine->setHidden(false);
    m_ui->wwwProxyExcludesLine->setToolTip(tr("Insert a list of addresses excluded from the proxy. E.g. \"internal.example.com 10.0.0.0/8\""));

    emit wwwProxyChanged(m_wwwProxy, m_wwwProxyServerList, m_wwwProxyExcludes);
}

void MerBuildEngineDetailsWidget::onWwwProxyServersEdited(const QString &s)
{
    QString proxies = s;
    QRegularExpression reg;

    int cpos = m_ui->wwwProxyServersLine->cursorPosition();
    int l = proxies.length();

    // Limit whitespace characters to spaces
    reg.setPattern("[\\s]");
    proxies.replace(reg, " ");
    if (m_wwwProxy == MER_SDK_PROXY_MANUAL) {
        // Multiple items are separated by a single space character
        // Clean up any extra whitespaces
        reg.setPattern("\\s{2,}");
        proxies.replace(reg, " ");
    }

    m_ui->wwwProxyServersLine->setText(proxies);
    m_ui->wwwProxyServersLine->textChanged(proxies);
    if (proxies.length() < l)
        cpos -= l - proxies.length();
    m_ui->wwwProxyServersLine->setCursorPosition(cpos);

    if (m_wwwProxy == MER_SDK_PROXY_AUTOMATIC) {
        m_wwwProxyServerUrl = proxies.trimmed();
        emit wwwProxyChanged(m_wwwProxy, m_wwwProxyServerUrl, QString());
    } else {
        m_wwwProxyServerList = proxies.trimmed();
        emit wwwProxyChanged(m_wwwProxy, m_wwwProxyServerList, m_wwwProxyExcludes);
    }
}

void MerBuildEngineDetailsWidget::onWwwProxyExcludesEdited(const QString &s)
{
    QString excludes = s;
    QRegularExpression reg;

    int cpos = m_ui->wwwProxyExcludesLine->cursorPosition();
    int l = excludes.length();

    // Limit whitespace characters to spaces
    reg.setPattern("[\\s]");
    excludes.replace(reg, " ");

    // Multiple items are separated by a single space character
    // Clean up any extra whitespaces
    reg.setPattern("\\s{2,}");
    excludes.replace(reg, " ");

    m_ui->wwwProxyExcludesLine->setText(excludes);
    m_ui->wwwProxyExcludesLine->textChanged(excludes);
    if (excludes.length() < l)
        cpos -= l - excludes.length();
    m_ui->wwwProxyExcludesLine->setCursorPosition(cpos);

    m_wwwProxyExcludes = excludes.trimmed();
    emit wwwProxyChanged(m_wwwProxy, m_wwwProxyServerList, m_wwwProxyExcludes);
}

} // Internal
} // Mer
