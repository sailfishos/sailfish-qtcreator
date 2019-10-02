/****************************************************************************
**
** Copyright (C) 2014-2016,2018-2019 Jolla Ltd.
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

#include "mergeneraloptionswidget.h"
#include "ui_mergeneraloptionswidget.h"

#include "merconstants.h"
#include "merdeployconfiguration.h"
#include "merdeploysteps.h"
#include "mersettings.h"

using namespace Utils;

namespace Mer {
namespace Internal {

MerGeneralOptionsWidget::MerGeneralOptionsWidget(QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui::MerGeneralOptionsWidget)
{
    m_ui->setupUi(this);

    m_ui->environmentFilterTextEdit->setPlainText(MerSettings::environmentFilter());
    m_ui->environmentFilterTextEdit->setEnabled(!MerSettings::isEnvironmentFilterFromEnvironment());
    m_ui->environmentFilterTextEdit->setToolTip(m_ui->environmentFilterTextEdit->toolTip()
            .arg(Constants::SAILFISH_SDK_ENVIRONMENT_FILTER));
    m_ui->environmentFilterOverriddenLabel->setVisible(MerSettings::isEnvironmentFilterFromEnvironment());
    m_ui->environmentFilterOverriddenLabel->setText(m_ui->environmentFilterOverriddenLabel->text()
            .arg(Constants::SAILFISH_SDK_ENVIRONMENT_FILTER));

    m_ui->rpmValidationByDefaultCheckBox->setToolTip(m_ui->rpmValidationByDefaultCheckBox->toolTip()
            .arg(MerRpmValidationStep::displayName())
            .arg(MerMb2RpmBuildConfigurationFactory::displayName()));
    m_ui->rpmValidationByDefaultCheckBox->setChecked(MerSettings::rpmValidationByDefault());

    m_ui->askBeforeStartingVmCheckBox->setChecked(MerSettings::isAskBeforeStartingVmEnabled());
    m_ui->askBeforeClosingVmCheckBox->setChecked(MerSettings::isAskBeforeClosingVmEnabled());
    m_ui->importQmakeVariablesCheckBox->setChecked(MerSettings::isImportQmakeVariablesEnabled());
    m_ui->askImportQmakeVariablesCheckBox->setEnabled(MerSettings::isImportQmakeVariablesEnabled());
    connect(m_ui->importQmakeVariablesCheckBox, &QCheckBox::toggled,
            m_ui->askImportQmakeVariablesCheckBox, &QCheckBox::setEnabled);
    m_ui->askImportQmakeVariablesCheckBox->setChecked(MerSettings::isAskImportQmakeVariablesEnabled());

    m_ui->benchLocationPathChooser->setExpectedKind(PathChooser::ExistingCommand);
    m_ui->benchLocationPathChooser->setPath(MerSettings::qmlLiveBenchLocation());
    m_ui->benchSyncWorkspaceCheckBox->setChecked(MerSettings::isSyncQmlLiveWorkspaceEnabled());

    m_ui->askImportQmakeVariablesCheckBox->setToolTip(tr("With this option disabled, qmake will be run without notice but you still may be prompted to start the\
        %1 build engine depending on the other options.").arg(QCoreApplication::translate("Mer", Mer::Constants::MER_OS_NAME)));
    m_ui->askBeforeStartingVmCheckBox->setToolTip(tr("Applies to starting a %1 build engine or emulator virtual machine during build,\
        deploy or run step execution.").arg(QCoreApplication::translate("Mer", Mer::Constants::MER_OS_NAME)));
    m_ui->askBeforeClosingVmCheckBox->setToolTip(tr("Applies to closing a headless %1 build engine virtual machine when \
        Qt Creator is about to quit.").arg(QCoreApplication::translate("Mer", Mer::Constants::MER_OS_NAME)));

    m_ui->rpmValidationInfoLabel->setText(tr("<html><head/><body><p><span style=\" font-style:italic;\">"
                      "Use the RPM Validator tool to do a quick quality criteria check for your %1 application package before publishing it. "
                      "The tool runs checks similar to the </span><a href=\"https://harbour.jolla.com/faq\"><span style=\" text-decoration: underline; color:#0000ff;\">"
                      "Jolla Harbour</span></a><span style=\" font-style:italic;\"> package validation process.</span></p></body></html>")
                      .arg(QCoreApplication::translate("Mer", Mer::Constants::MER_OS_NAME)));
}

MerGeneralOptionsWidget::~MerGeneralOptionsWidget()
{
    delete m_ui;
}

void MerGeneralOptionsWidget::store()
{
    if (!MerSettings::isEnvironmentFilterFromEnvironment())
        MerSettings::setEnvironmentFilter(m_ui->environmentFilterTextEdit->toPlainText());
    MerSettings::setRpmValidationByDefault(m_ui->rpmValidationByDefaultCheckBox->isChecked());
    MerSettings::setAskBeforeStartingVmEnabled(m_ui->askBeforeStartingVmCheckBox->isChecked());
    MerSettings::setAskBeforeClosingVmEnabled(m_ui->askBeforeClosingVmCheckBox->isChecked());
    MerSettings::setQmlLiveBenchLocation(m_ui->benchLocationPathChooser->path());
    MerSettings::setSyncQmlLiveWorkspaceEnabled(m_ui->benchSyncWorkspaceCheckBox->isChecked());
    MerSettings::setImportQmakeVariablesEnabled(m_ui->importQmakeVariablesCheckBox->isChecked());
    MerSettings::setAskImportQmakeVariablesEnabled(m_ui->askImportQmakeVariablesCheckBox->isChecked());
}

QString MerGeneralOptionsWidget::searchKeywords() const
{
    QString keywords;
    const QLatin1Char sep(' ');
    QTextStream(&keywords) << sep << m_ui->environmentFilterLabel->text()
                           << sep << m_ui->rpmValidationInfoLabel->text()
                           << sep << m_ui->rpmValidationByDefaultCheckBox->text()
                           << sep << m_ui->askBeforeStartingVmCheckBox->text()
                           << sep << m_ui->qmlLiveGroupBox->title()
                           << sep << m_ui->benchLocationLabel->text()
                           << sep << m_ui->importQmakeVariablesCheckBox->text()
                           << sep << m_ui->askImportQmakeVariablesCheckBox->text()
                           ;
    keywords.remove(QLatin1Char('&'));
    return keywords;
}

} // Internal
} // Mer
