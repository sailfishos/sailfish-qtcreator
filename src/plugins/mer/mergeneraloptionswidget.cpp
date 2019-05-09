/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
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
            .arg(MerMb2RpmBuildConfiguration::displayName()));
    m_ui->rpmValidationByDefaultCheckBox->setChecked(MerSettings::rpmValidationByDefault());

    m_ui->askBeforeStartingVmCheckBox->setChecked(MerSettings::isAskBeforeStartingVmEnabled());
    m_ui->askBeforeClosingVmCheckBox->setChecked(MerSettings::isAskBeforeClosingVmEnabled());
    m_ui->importQmakeVariablesCheckBox->setChecked(MerSettings::isImportQmakeVariablesEnabled());

    m_ui->benchLocationPathChooser->setExpectedKind(PathChooser::ExistingCommand);
    m_ui->benchLocationPathChooser->setPath(MerSettings::qmlLiveBenchLocation());
    m_ui->benchSyncWorkspaceCheckBox->setChecked(MerSettings::isSyncQmlLiveWorkspaceEnabled());
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
                           ;
    keywords.remove(QLatin1Char('&'));
    return keywords;
}

} // Internal
} // Mer
