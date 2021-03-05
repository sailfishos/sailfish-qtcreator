/****************************************************************************
**
** Copyright (C) 2016 BlackBerry Limited. All rights reserved.
** Contact: BlackBerry (qt@blackberry.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "qnxsettingspage.h"

#include "ui_qnxsettingswidget.h"
#include "qnxconfiguration.h"
#include "qnxconfigurationmanager.h"

#include <coreplugin/icore.h>

#include <projectexplorer/projectexplorerconstants.h>

#include <qtsupport/qtversionmanager.h>

#include <QFileDialog>
#include <QMessageBox>

namespace Qnx {
namespace Internal {

class QnxSettingsWidget final : public Core::IOptionsPageWidget
{
    Q_DECLARE_TR_FUNCTIONS(Qnx::Internal::QnxSettingsWidget)

public:
    QnxSettingsWidget();

    enum State {
        Activated,
        Deactivated,
        Added,
        Removed
    };

    class ConfigState {
    public:
        bool operator ==(const ConfigState &cs) const
        {
            return config == cs.config && state == cs.state;
        }

        QnxConfiguration *config;
        State state;
    };

    void apply() final;

    void addConfiguration();
    void removeConfiguration();
    void generateKits(bool checked);
    void updateInformation();
    void populateConfigsCombo();

    void setConfigState(QnxConfiguration *config, State state);

private:
    Ui_QnxSettingsWidget m_ui;
    QnxConfigurationManager *m_qnxConfigManager;
    QList<ConfigState> m_changedConfigs;
};

QnxSettingsWidget::QnxSettingsWidget() :
    m_qnxConfigManager(QnxConfigurationManager::instance())
{
    m_ui.setupUi(this);

    populateConfigsCombo();
    connect(m_ui.addButton, &QAbstractButton::clicked,
            this, &QnxSettingsWidget::addConfiguration);
    connect(m_ui.removeButton, &QAbstractButton::clicked,
            this, &QnxSettingsWidget::removeConfiguration);
    connect(m_ui.configsCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &QnxSettingsWidget::updateInformation);
    connect(m_ui.generateKitsCheckBox, &QAbstractButton::toggled,
            this, &QnxSettingsWidget::generateKits);
    connect(m_qnxConfigManager, &QnxConfigurationManager::configurationsListUpdated,
            this, &QnxSettingsWidget::populateConfigsCombo);
    connect(QtSupport::QtVersionManager::instance(),
            &QtSupport::QtVersionManager::qtVersionsChanged,
            this, &QnxSettingsWidget::updateInformation);
}

void QnxSettingsWidget::addConfiguration()
{
    QString filter;
    if (Utils::HostOsInfo::isWindowsHost())
        filter = QLatin1String("*.bat file");
    else
        filter = QLatin1String("*.sh file");

    const QString envFile = QFileDialog::getOpenFileName(this, tr("Select QNX Environment File"),
                                                         QString(), filter);
    if (envFile.isEmpty())
        return;

    QnxConfiguration *config = new QnxConfiguration(Utils::FilePath::fromString(envFile));
    if (m_qnxConfigManager->configurations().contains(config)
            || !config->isValid()) {
        QMessageBox::warning(Core::ICore::dialogParent(),
                             tr("Warning"),
                             tr("Configuration already exists or is invalid."));
        delete config;
        return;
    }

    setConfigState(config, Added);
    m_ui.configsCombo->addItem(config->displayName(),
                                   QVariant::fromValue(static_cast<void*>(config)));
}

void QnxSettingsWidget::removeConfiguration()
{
    const int currentIndex = m_ui.configsCombo->currentIndex();
    auto config = static_cast<QnxConfiguration*>(
            m_ui.configsCombo->itemData(currentIndex).value<void*>());

    if (!config)
        return;

    QMessageBox::StandardButton button =
            QMessageBox::question(Core::ICore::dialogParent(),
                                  tr("Remove QNX Configuration"),
                                  tr("Are you sure you want to remove:\n %1?").arg(config->displayName()),
                                  QMessageBox::Yes | QMessageBox::No);

    if (button == QMessageBox::Yes) {
        setConfigState(config, Removed);
        m_ui.configsCombo->removeItem(currentIndex);
    }
}

void QnxSettingsWidget::generateKits(bool checked)
{
    const int currentIndex = m_ui.configsCombo->currentIndex();
    auto config = static_cast<QnxConfiguration*>(
                m_ui.configsCombo->itemData(currentIndex).value<void*>());
    if (!config)
        return;

    setConfigState(config, checked ? Activated : Deactivated);
}

void QnxSettingsWidget::updateInformation()
{
    const int currentIndex = m_ui.configsCombo->currentIndex();

    auto config = static_cast<QnxConfiguration*>(
            m_ui.configsCombo->itemData(currentIndex).value<void*>());

    // update the checkbox
    m_ui.generateKitsCheckBox->setEnabled(config ? config->canCreateKits() : false);
    m_ui.generateKitsCheckBox->setChecked(config ? config->isActive() : false);

    // update information
    m_ui.configName->setText(config ? config->displayName() : QString());
    m_ui.configVersion->setText(config ? config->version().toString() : QString());
    m_ui.configHost->setText(config ? config->qnxHost().toString() : QString());
    m_ui.configTarget->setText(config ? config->qnxTarget().toString() : QString());
}

void QnxSettingsWidget::populateConfigsCombo()
{
    m_ui.configsCombo->clear();
    foreach (QnxConfiguration *config, m_qnxConfigManager->configurations()) {
        m_ui.configsCombo->addItem(config->displayName(),
                                       QVariant::fromValue(static_cast<void*>(config)));
    }

    updateInformation();
}

void QnxSettingsWidget::setConfigState(QnxConfiguration *config, State state)
{
    State stateToRemove = Activated;
    switch (state) {
    case Added :
        stateToRemove = Removed;
        break;
    case Removed:
        stateToRemove = Added;
        break;
    case Activated:
        stateToRemove = Deactivated;
        break;
    case Deactivated:
        stateToRemove = Activated;
        break;
    }

    for (const ConfigState &configState : qAsConst(m_changedConfigs)) {
        if (configState.config == config && configState.state == stateToRemove)
            m_changedConfigs.removeAll(configState);
    }

     m_changedConfigs.append(ConfigState{config, state});
}

void QnxSettingsWidget::apply()
{
    for (const ConfigState &configState : qAsConst(m_changedConfigs)) {
        switch (configState.state) {
        case Activated :
            configState.config->activate();
            break;
        case Deactivated:
            configState.config->deactivate();
            break;
        case Added:
            m_qnxConfigManager->addConfiguration(configState.config);
            break;
        case Removed:
            configState.config->deactivate();
            m_qnxConfigManager->removeConfiguration(configState.config);
            break;
        }
    }

    m_changedConfigs.clear();
}


// QnxSettingsPage

QnxSettingsPage::QnxSettingsPage()
{
    setId("DD.Qnx Configuration");
    setDisplayName(QnxSettingsWidget::tr("QNX"));
    setCategory(ProjectExplorer::Constants::DEVICE_SETTINGS_CATEGORY);
    setWidgetCreator([] { return new QnxSettingsWidget; });
}

} // namespace Internal
} // namespace Qnx
