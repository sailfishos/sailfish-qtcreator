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

#ifndef MERDEVICEMODELCOMBOBOX_H
#define MERDEVICEMODELCOMBOBOX_H

#include "meremulatordevice.h"

#include <QComboBox>
#include <QStandardItemModel>

namespace Mer {
namespace Internal {

class MerDeviceModelComboBox : public QComboBox
{
    Q_OBJECT
public:
    explicit MerDeviceModelComboBox(QWidget *parent = nullptr);
    ~MerDeviceModelComboBox();

    QString currentDeviceModel() const;
    void setDeviceModels(const QList<MerEmulatorDeviceModel> &models);

public slots:
    void setCurrentDeviceModel(const QString &name);

private slots:
    void updateToolTip();

private:
    static QString deviceModelInfo(const MerEmulatorDeviceModel &model);

    QStandardItemModel m_model;
};

} // Internal
} // Mer

#endif // MERDEVICEMODELCOMBOBOX_H
