/****************************************************************************
**
** Copyright (C) 2012 - 2019 Jolla Ltd.
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

#ifndef MEREMULATORMODEDETAILSWIDGET_H
#define MEREMULATORMODEDETAILSWIDGET_H

#include "meremulatordevice.h"

#include <QWidget>

namespace Mer {
namespace Internal {

namespace Ui {
class MerEmulatorModeDetailsWidget;
}

class MerEmulatorModeDetailsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MerEmulatorModeDetailsWidget(QWidget *parent = nullptr);
    ~MerEmulatorModeDetailsWidget();

    void setCurrentDeviceModel(const MerEmulatorDeviceModel &model);
    void setDeviceModelsNames(const QStringList &deviceModelsNames);

signals:
    void deviceModelNameChanged(const QString &name);
    void screenResolutionChanged(const QSize &resolution);
    void screenSizeChanged(const QSize &size);
    void dconfChanged(const QString &dconf);

private slots:
    void onSimpleOptionsChanged();
    void onAdvancedOptionsChanged();
    void onModeChanged(int tabIndex);
    void onScreenResolutionChanged();
    void onScreenSizeChanged();
    void onDeviceModelNameChanged();

private:
    void updateNameLineEdit();
    void updateTextEdit(const QString &name, const QVariant &value);
    QVariant getTextEditValue(const QString &name) const;

    Ui::MerEmulatorModeDetailsWidget *ui;
    QString m_deviceModelName;
    QStringList m_deviceModelsNames;
};

} // namespace Internal
} // namespace Mer

#endif // MEREMULATORMODEDETAILSWIDGET_H
