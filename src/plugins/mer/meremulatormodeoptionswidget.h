/****************************************************************************
**
** Copyright (C) 2012 - 2018 Jolla Ltd.
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

#ifndef MEREMULATORMODEOPTIONSWIDGET_H
#define MEREMULATORMODEOPTIONSWIDGET_H

#include "meremulatordevice.h"

#include <QWidget>

namespace Mer {
namespace Internal {

namespace Ui {
class MerEmulatorModeOptionsWidget;
}

class MerEmulatorModeOptionsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MerEmulatorModeOptionsWidget(QWidget *parent = nullptr);
    ~MerEmulatorModeOptionsWidget();

    void store();
    QString searchKeywords() const;

private slots:
    void updateGui();
    void deviceModelScreenResolutionChanged(const QSize &resolution);
    void deviceModelScreenSizeChanged(const QSize &size);
    void deviceModelNameChanged(const QString &newName);
    void deviceModelDconfChanged(const QString &value);
    void on_addProfileButton_clicked();
    void on_removeProfileButton_clicked();

private:
    bool updateDeviceModel(const MerEmulatorDeviceModel &model);
    bool addDeviceModel(const QString &name);
    bool removeDeviceModel(const QString &name);
    bool renameDeviceModel(const QString &name, const QString &newName);

    Ui::MerEmulatorModeOptionsWidget *m_ui;
    QMap<QString, MerEmulatorDeviceModel> m_deviceModels;
};

} // Internal
} // Mer

#endif // MEREMULATORMODEOPTIONSWIDGET_H
