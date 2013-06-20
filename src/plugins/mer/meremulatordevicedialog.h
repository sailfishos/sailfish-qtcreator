/****************************************************************************
**
** Copyright (C) 2012 - 2013 Jolla Ltd.
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

#ifndef MEREMULATORDEVICEDIALOG_H
#define MEREMULATORDEVICEDIALOG_H

#include <QDialog>

namespace Mer {
namespace Internal {

namespace Ui { class MerEmulatorDeviceDialog; }

class MerEmulatorDeviceDialog : public QDialog
{
    Q_OBJECT
public:
    MerEmulatorDeviceDialog(QWidget *parent = 0);
    ~MerEmulatorDeviceDialog();

    QString configName() const;
    QString userName() const;
    int sshPort() const;
    int timeout() const;
    QString privateKey() const;
    QString emulatorVm() const;
    QString sdkVm() const;
    QString freePorts() const;
    QString sharedConfigPath() const;
    QString sharedSshPath() const;
    int index() const;
    bool requireSshKeys() const;

private slots:
    void handleKeyChanged();
    void handleEmulatorVmChanged(const QString &vmName);
private:
     Ui::MerEmulatorDeviceDialog *m_ui;
     int m_index;
};

}
}
#endif // MEREMULATORDEVICEDIALOG_H
