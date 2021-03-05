/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
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

#include "clangprojectsettingswidget.h"

#include "clangmodelmanagersupport.h"
#include "clangprojectsettings.h"

#include <coreplugin/icore.h>

#include <cpptools/clangdiagnosticconfig.h>
#include <cpptools/clangdiagnosticconfigswidget.h>
#include <cpptools/cppcodemodelsettings.h>
#include <cpptools/cpptoolsconstants.h>
#include <cpptools/cpptoolsreuse.h>

#include <utils/hostosinfo.h>

namespace ClangCodeModel {
namespace Internal {

static Utils::Id configIdForProject(ClangProjectSettings &projectSettings)
{
    if (projectSettings.useGlobalConfig())
        return CppTools::codeModelSettings()->clangDiagnosticConfigId();
    return projectSettings.warningConfigId();
}

ClangProjectSettingsWidget::ClangProjectSettingsWidget(ProjectExplorer::Project *project)
    : m_projectSettings(ClangModelManagerSupport::instance()->projectSettings(project))
{
    m_ui.setupUi(this);

    using namespace CppTools;

    m_ui.delayedTemplateParseCheckBox->setVisible(Utils::HostOsInfo::isWindowsHost());

    // Links
    connect(m_ui.gotoGlobalSettingsLabel, &QLabel::linkActivated, [](const QString &) {
        Core::ICore::showOptionsDialog(CppTools::Constants::CPP_CODE_MODEL_SETTINGS_ID);
    });

    connect(m_ui.clangDiagnosticConfigsSelectionWidget,
            &ClangDiagnosticConfigsSelectionWidget::changed,
            this,
            [this]() {
                // Save project's config id
                const Utils::Id currentConfigId = m_ui.clangDiagnosticConfigsSelectionWidget
                                                     ->currentConfigId();
                m_projectSettings.setWarningConfigId(currentConfigId);

                // Save global custom configs
                const ClangDiagnosticConfigs configs = m_ui.clangDiagnosticConfigsSelectionWidget
                                                           ->customConfigs();
                CppTools::codeModelSettings()->setClangCustomDiagnosticConfigs(configs);
                CppTools::codeModelSettings()->toSettings(Core::ICore::settings());
            });

    connect(m_ui.delayedTemplateParseCheckBox, &QCheckBox::toggled,
            this, &ClangProjectSettingsWidget::onDelayedTemplateParseClicked);
    connect(m_ui.globalOrCustomComboBox,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ClangProjectSettingsWidget::onGlobalCustomChanged);
    connect(project, &ProjectExplorer::Project::aboutToSaveSettings,
            this, &ClangProjectSettingsWidget::onAboutToSaveProjectSettings);

    connect(&m_projectSettings, &ClangProjectSettings::changed,
            this, &ClangProjectSettingsWidget::syncWidgets);
    connect(CppTools::codeModelSettings(), &CppTools::CppCodeModelSettings::changed,
            this, &ClangProjectSettingsWidget::syncOtherWidgetsToComboBox);

    syncWidgets();
}

void ClangProjectSettingsWidget::onDelayedTemplateParseClicked(bool checked)
{
    // Don't save it when we reset the global config in code
    if (m_projectSettings.useGlobalConfig())
        return;

    const QLatin1String extraFlag{checked ? ClangProjectSettings::DelayedTemplateParsing
                                          : ClangProjectSettings::NoDelayedTemplateParsing};
    QStringList options = m_projectSettings.commandLineOptions();
    options.removeAll(QLatin1String{ClangProjectSettings::DelayedTemplateParsing});
    options.removeAll(QLatin1String{ClangProjectSettings::NoDelayedTemplateParsing});
    options.append(extraFlag);
    m_projectSettings.setCommandLineOptions(options);
}

void ClangProjectSettingsWidget::onGlobalCustomChanged(int index)
{
    m_projectSettings.setUseGlobalConfig(index == 0 ? true : false);
    syncOtherWidgetsToComboBox();
}

void ClangProjectSettingsWidget::onAboutToSaveProjectSettings()
{
    CppTools::codeModelSettings()->toSettings(Core::ICore::settings());
}

void ClangProjectSettingsWidget::syncWidgets()
{
    syncGlobalCustomComboBox();
    syncOtherWidgetsToComboBox();
}

void ClangProjectSettingsWidget::syncGlobalCustomComboBox()
{
    m_ui.globalOrCustomComboBox->setCurrentIndex(m_projectSettings.useGlobalConfig() ? 0 : 1);
}

void ClangProjectSettingsWidget::syncOtherWidgetsToComboBox()
{
    const QStringList options = m_projectSettings.commandLineOptions();
    m_ui.delayedTemplateParseCheckBox->setChecked(
        options.contains(QLatin1String{ClangProjectSettings::DelayedTemplateParsing}));

    const bool isCustom = !m_projectSettings.useGlobalConfig();
    m_ui.delayedTemplateParseCheckBox->setEnabled(isCustom);

    for (int i = 0; i < m_ui.clangDiagnosticConfigsSelectionWidget->layout()->count(); ++i) {
        QWidget *widget = m_ui.clangDiagnosticConfigsSelectionWidget->layout()->itemAt(i)->widget();
        if (widget)
            widget->setEnabled(isCustom);
    }

    m_ui.clangDiagnosticConfigsSelectionWidget
        ->refresh(CppTools::diagnosticConfigsModel(),
                  configIdForProject(m_projectSettings),
                  [](const CppTools::ClangDiagnosticConfigs &configs,
                     const Utils::Id &configToSelect) {
                      return new CppTools::ClangDiagnosticConfigsWidget(configs, configToSelect);
                  });
}

} // namespace Internal
} // namespace ClangCodeModel
