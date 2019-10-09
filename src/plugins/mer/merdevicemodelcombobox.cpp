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

#include "merdevicemodelcombobox.h"
#include "mersettings.h"

#include <sfdk/emulator.h>
#include <sfdk/sdk.h>

#include <QCollator>

using namespace Sfdk;

namespace Mer {
namespace Internal {

namespace {

const int deviceModelNameRole = Qt::UserRole;

class StandardItem : public QStandardItem
{
public:
    StandardItem(const QString &text)
        : QStandardItem(text)
        , m_collator(QLocale::system())
    {
        m_collator.setCaseSensitivity(Qt::CaseInsensitive);
        m_collator.setNumericMode(true);
    }
    bool operator<(const QStandardItem &other) const
    {
        const QString name1 = data(deviceModelNameRole).toString();
        const QString name2 = other.data(deviceModelNameRole).toString();
        return m_collator.compare(name1, name2) < 0;
    }
private:
    QCollator m_collator;
};

} // namespace anonymous

MerDeviceModelComboBox::MerDeviceModelComboBox(QWidget *parent)
    : QComboBox(parent)
    , m_model(0, 1)
{
    setModel(&m_model);

    connect(this, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MerDeviceModelComboBox::updateToolTip);
}

MerDeviceModelComboBox::~MerDeviceModelComboBox()
{
}

void MerDeviceModelComboBox::updateToolTip()
{
    setToolTip(currentData(Qt::ToolTipRole).toString());
}

QString MerDeviceModelComboBox::deviceModelInfo(const Sfdk::DeviceModelData &model)
{
    const bool isStored = Sdk::deviceModel(model.name) == model;
    return QStringLiteral("%1 (%2x%3)%4")
            .arg(model.name)
            .arg(model.displayResolution.width())
            .arg(model.displayResolution.height())
            .arg(isStored ? QString() : QString(" *"));
}

QString MerDeviceModelComboBox::currentDeviceModel() const
{
    return currentData(deviceModelNameRole).toString();
}

void MerDeviceModelComboBox::setCurrentDeviceModel(const QString &name)
{
    const int index = findData(name, deviceModelNameRole);
    setCurrentIndex(index);
}

void MerDeviceModelComboBox::setDeviceModels(const QList<Sfdk::DeviceModelData> &models)
{
    const bool state = blockSignals(true);

    const QString name = currentDeviceModel();
    clear();
    for (const DeviceModelData &deviceModel : models) {
        m_model.appendRow(new StandardItem(deviceModelInfo(deviceModel)));
        QFont f = font();
        f.setItalic(!deviceModel.autodetected);
        const int last = count() - 1;
        setItemData(last, deviceModel.name, deviceModelNameRole);
        setItemData(last, f, Qt::FontRole);
        setItemData(last, deviceModel.autodetected
                    ? tr("SDK provided device model")
                    : tr("User defined device model"), Qt::ToolTipRole);
        updateToolTip();
    }
    model()->sort(0);
    setCurrentDeviceModel(name);

    blockSignals(state);
}

} // Internal
} // Mer
