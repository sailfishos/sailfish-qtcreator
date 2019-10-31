/****************************************************************************
**
** Copyright (C) 2019 Jolla Ltd.
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

#include "meremulatormodeoptionswidget.h"
#include "ui_meremulatormodeoptionswidget.h"

#include "mersettings.h"

#include <sfdk/emulator.h>
#include <sfdk/sdk.h>

#include <utils/qtcassert.h>
#include <utils/stringutils.h>

#include <QMessageBox>

using namespace Sfdk;

namespace Mer {
namespace Internal {

MerEmulatorModeOptionsWidget::MerEmulatorModeOptionsWidget(QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui::MerEmulatorModeOptionsWidget)
    , m_deviceModels(toMap(Sdk::deviceModels()))
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
    // FIXME progress bar
    bool ok;
    execAsynchronous(std::tie(ok), Sdk::setDeviceModels, m_deviceModels.values());
    QTC_CHECK(ok);

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

bool MerEmulatorModeOptionsWidget::updateDeviceModel(const DeviceModelData &model)
{
    QTC_ASSERT(!model.autodetected, return false);

    m_deviceModels.insert(model.name, model);

    m_ui->deviceModelComboBox->setDeviceModels(m_deviceModels.values());

    return true;
}

bool MerEmulatorModeOptionsWidget::renameDeviceModel(const QString &name, const QString &newName)
{
    QTC_ASSERT(name != newName, return false);
    QTC_ASSERT(m_deviceModels.count(name) == 1, return false);
    QTC_ASSERT(!m_deviceModels.contains(newName), return false);

    DeviceModelData model = m_deviceModels.value(name);
    QTC_ASSERT(!model.autodetected, return false);

    m_deviceModels.remove(name);
    model.name = newName;
    m_deviceModels.insert(newName, model);

    m_ui->deviceModelComboBox->setDeviceModels(m_deviceModels.values());
    m_ui->deviceModelComboBox->setCurrentDeviceModel(newName);
    m_ui->optionsWidget->setDeviceModelsNames(m_deviceModels.uniqueKeys());

    return true;
}

bool MerEmulatorModeOptionsWidget::addDeviceModel(const QString &name)
{
    QTC_ASSERT(!m_deviceModels.contains(name), return false);

    DeviceModelData model;
    model.name = name;
    model.displayResolution = QSize(500, 900);
    model.displaySize = QSize(50, 90);
    m_deviceModels.insert(name, model);

    m_ui->deviceModelComboBox->setDeviceModels(m_deviceModels.values());
    m_ui->deviceModelComboBox->setCurrentDeviceModel(name);
    m_ui->optionsWidget->setCurrentDeviceModel(m_deviceModels.value(name));

    return true;
}

bool MerEmulatorModeOptionsWidget::removeDeviceModel(const QString &name)
{
    QTC_ASSERT(m_deviceModels.contains(name), return false);
    QTC_ASSERT(!m_deviceModels.value(name).autodetected, return false);

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
    const DeviceModelData model = m_deviceModels.value(name);
    QTC_ASSERT(!model.isNull(), return);

    if (model.dconf != value) {
        auto copy = model;
        copy.dconf = value;
        updateDeviceModel(copy);
    }
}

void MerEmulatorModeOptionsWidget::deviceModelScreenResolutionChanged(const QSize &resolution)
{
    const auto name = m_ui->deviceModelComboBox->currentDeviceModel();
    const DeviceModelData model = m_deviceModels.value(name);
    QTC_ASSERT(!model.isNull(), return);

    if (model.displayResolution != resolution) {
        auto copy = model;
        copy.displayResolution = resolution;
        updateDeviceModel(copy);
    }
}

void MerEmulatorModeOptionsWidget::deviceModelScreenSizeChanged(const QSize &size)
{
    const auto name = m_ui->deviceModelComboBox->currentDeviceModel();
    const DeviceModelData model = m_deviceModels.value(name);
    QTC_ASSERT(!model.isNull(), return);

    if (model.displaySize != size) {
        auto copy = model;
        copy.displaySize = size;
        updateDeviceModel(copy);
    }
}

void MerEmulatorModeOptionsWidget::updateGui()
{
    const QString name = m_ui->deviceModelComboBox->currentDeviceModel();
    const DeviceModelData model = m_deviceModels.value(name);
    m_ui->removeProfileButton->setDisabled(model.autodetected);
    m_ui->optionsWidget->setCurrentDeviceModel(model);
}

void MerEmulatorModeOptionsWidget::on_addProfileButton_clicked()
{
    QTC_CHECK(m_deviceModels.uniqueKeys()
              == m_deviceModels.keys());

    // Find number for new device model
    const QSet<QString> names = m_deviceModels.keys().toSet();
    const QString name = Utils::makeUniquelyNumbered(tr("Unnamed"), names);
    addDeviceModel(name);
}

void MerEmulatorModeOptionsWidget::on_removeProfileButton_clicked()
{
    const QString name = m_ui->deviceModelComboBox->currentDeviceModel();
    removeDeviceModel(name);
}

QMap<QString, Sfdk::DeviceModelData> MerEmulatorModeOptionsWidget::toMap(
        const QList<Sfdk::DeviceModelData> &deviceModels)
{
    QMap<QString, DeviceModelData> map;
    for (const DeviceModelData &deviceModel : deviceModels)
        map.insert(deviceModel.name, deviceModel);
    return map;
}

} // Internal
} // Mer
