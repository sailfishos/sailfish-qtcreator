/****************************************************************************
**
** Copyright (C) 2019 Jolla Ltd.
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

#include "meremulatormodeoptionswidget.h"
#include "ui_meremulatormodeoptionswidget.h"

#include "mersettings.h"

#include <utils/qtcassert.h>

#include <QMessageBox>

namespace Mer {
namespace Internal {

MerEmulatorModeOptionsWidget::MerEmulatorModeOptionsWidget(QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui::MerEmulatorModeOptionsWidget)
    , m_deviceModels(MerSettings::deviceModels())
{
    m_ui->setupUi(this);
    m_ui->optionsWidget->setDeviceModelsNames(m_deviceModels.uniqueKeys());

    connect(m_ui->optionsWidget, &MerEmulatorModeDetailsWidget::screenResolutionChanged,
            this, &MerEmulatorModeOptionsWidget::deviceModelScreenResolutionChanged);
    connect(m_ui->optionsWidget, &MerEmulatorModeDetailsWidget::screenSizeChanged,
            this, &MerEmulatorModeOptionsWidget::deviceModelScreenSizeChanged);
    connect(m_ui->optionsWidget, &MerEmulatorModeDetailsWidget::dconfChanged,
            this, &MerEmulatorModeOptionsWidget::deviceModelDconfChanged);
    connect(m_ui->optionsWidget, &MerEmulatorModeDetailsWidget::deviceModelNameChanged,
            this, &MerEmulatorModeOptionsWidget::deviceModelNameChanged);
    connect(m_ui->deviceModelComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MerEmulatorModeOptionsWidget::updateGui);

    m_ui->deviceModelComboBox->setDeviceModels(m_deviceModels.values());
    m_ui->deviceModelComboBox->setCurrentIndex(0);
}

MerEmulatorModeOptionsWidget::~MerEmulatorModeOptionsWidget()
{
    delete m_ui;
}

void MerEmulatorModeOptionsWidget::store()
{
    MerSettings::setDeviceModels(m_deviceModels);

    m_ui->optionsWidget->setDeviceModelsNames(m_deviceModels.uniqueKeys());
    m_ui->deviceModelComboBox->setDeviceModels(m_deviceModels.values());
    updateGui();
}

QString MerEmulatorModeOptionsWidget::searchKeywords() const
{
    const QStringList keywordsList {
        m_ui->deviceModelLabel->text(),
        m_ui->addProfileButton->toolTip(),
        m_ui->removeProfileButton->toolTip()
    };

    QString keywords = keywordsList.join(' ');
    keywords.remove(QLatin1Char('&'));
    return keywords;
}

bool MerEmulatorModeOptionsWidget::updateDeviceModel(const MerEmulatorDeviceModel &model)
{
    QTC_ASSERT(!model.isSdkProvided(), return false);

    m_deviceModels.insert(model.name(), model);

    m_ui->deviceModelComboBox->setDeviceModels(m_deviceModels.values());

    return true;
}

bool MerEmulatorModeOptionsWidget::renameDeviceModel(const QString &name, const QString &newName)
{
    QTC_ASSERT(name != newName, return false);
    QTC_ASSERT(m_deviceModels.count(name) == 1, return false);
    QTC_ASSERT(!m_deviceModels.contains(newName), return false);

    MerEmulatorDeviceModel model = m_deviceModels.value(name);
    QTC_ASSERT(!model.isSdkProvided(), return false);

    m_deviceModels.remove(name);
    model.setName(newName);
    m_deviceModels.insert(newName, model);

    m_ui->deviceModelComboBox->setDeviceModels(m_deviceModels.values());
    m_ui->deviceModelComboBox->setCurrentDeviceModel(newName);
    m_ui->optionsWidget->setDeviceModelsNames(m_deviceModels.uniqueKeys());

    return true;
}

bool MerEmulatorModeOptionsWidget::addDeviceModel(const QString &name)
{
    QTC_ASSERT(!m_deviceModels.contains(name), return false);

    MerEmulatorDeviceModel emulatorProfile(name, QSize(500, 900), QSize(50, 90));
    m_deviceModels.insert(name, emulatorProfile);

    m_ui->deviceModelComboBox->setDeviceModels(m_deviceModels.values());
    m_ui->deviceModelComboBox->setCurrentDeviceModel(name);
    m_ui->optionsWidget->setCurrentDeviceModel(m_deviceModels.value(name));

    return true;
}

bool MerEmulatorModeOptionsWidget::removeDeviceModel(const QString &name)
{
    QTC_ASSERT(m_deviceModels.contains(name), return false);
    QTC_ASSERT(!m_deviceModels.value(name).isSdkProvided(), return false);

    m_deviceModels.remove(name);

    m_ui->deviceModelComboBox->setDeviceModels(m_deviceModels.values());
    m_ui->deviceModelComboBox->setCurrentIndex(0);
    m_ui->optionsWidget->setDeviceModelsNames(m_deviceModels.uniqueKeys());

    return true;
}

void MerEmulatorModeOptionsWidget::deviceModelNameChanged(const QString &newName)
{
    const QString name = m_ui->deviceModelComboBox->currentDeviceModel();
    renameDeviceModel(name, newName);
}

void MerEmulatorModeOptionsWidget::deviceModelDconfChanged(const QString &value)
{
    const QString name = m_ui->deviceModelComboBox->currentDeviceModel();
    const MerEmulatorDeviceModel model = m_deviceModels.value(name);
    QTC_ASSERT(!model.isNull(), return);

    if (model.dconf() != value) {
        updateDeviceModel(MerEmulatorDeviceModel(name,
                                                 model.displayResolution(),
                                                 model.displaySize(),
                                                 value,
                                                 model.isSdkProvided()));
    }
}

void MerEmulatorModeOptionsWidget::deviceModelScreenResolutionChanged(const QSize &resolution)
{
    const auto name = m_ui->deviceModelComboBox->currentDeviceModel();
    const MerEmulatorDeviceModel model = m_deviceModels.value(name);
    QTC_ASSERT(!model.isNull(), return);

    if (model.displayResolution() != resolution) {
        updateDeviceModel(MerEmulatorDeviceModel(name,
                                                 resolution,
                                                 model.displaySize(),
                                                 model.dconf(),
                                                 model.isSdkProvided()));
    }
}

void MerEmulatorModeOptionsWidget::deviceModelScreenSizeChanged(const QSize &size)
{
    const auto name = m_ui->deviceModelComboBox->currentDeviceModel();
    const MerEmulatorDeviceModel model = m_deviceModels.value(name);
    QTC_ASSERT(!model.isNull(), return);

    if (model.displaySize() != size) {
        updateDeviceModel(MerEmulatorDeviceModel(name,
                                                 model.displayResolution(),
                                                 size,
                                                 model.dconf(),
                                                 model.isSdkProvided()));
    }
}

void MerEmulatorModeOptionsWidget::updateGui()
{
    const QString name = m_ui->deviceModelComboBox->currentDeviceModel();
    const MerEmulatorDeviceModel model = m_deviceModels.value(name);
    m_ui->removeProfileButton->setDisabled(model.isSdkProvided());
    m_ui->optionsWidget->setCurrentDeviceModel(model);
}

void MerEmulatorModeOptionsWidget::on_addProfileButton_clicked()
{
    QTC_CHECK(m_deviceModels.uniqueKeys()
              == m_deviceModels.keys());

    // Find number for new device model
    const QSet<QString> names = m_deviceModels.keys().toSet();
    const QString name = MerEmulatorDeviceModel::uniqueName(tr("Unnamed"), names);
    addDeviceModel(name);
}

void MerEmulatorModeOptionsWidget::on_removeProfileButton_clicked()
{
    const QString name = m_ui->deviceModelComboBox->currentDeviceModel();
    removeDeviceModel(name);
}

} // Internal
} // Mer
