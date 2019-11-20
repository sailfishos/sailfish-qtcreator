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

#ifndef MERVIRTUALMACHINESETTINGSWIDGET_H
#define MERVIRTUALMACHINESETTINGSWIDGET_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QFormLayout;
QT_END_NAMESPACE

namespace Mer {
namespace Internal {

namespace Ui {
class MerVirtualMachineSettingsWidget;
}

class MerVirtualMachineSettingsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MerVirtualMachineSettingsWidget(QWidget *parent = nullptr);
    ~MerVirtualMachineSettingsWidget();
    void setMemorySizeMb(int sizeMb);
    void setCpuCount(int count);
    void setStorageSizeMb(int storageSizeMb);
    void setVmOff(bool vmOff);
    QFormLayout *formLayout() const;

signals:
    void memorySizeMbChanged(int sizeMb);
    void cpuCountChanged(int count);
    void storageSizeMbChnaged(int sizeMb);

private:
    void initGui();
    Ui::MerVirtualMachineSettingsWidget *ui;
};

} // Internal
} // Mer

#endif // MERVIRTUALMACHINESETTINGSWIDGET_H
