/****************************************************************************
**
** Copyright (C) 2012 - 2014 Jolla Ltd.
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

#ifndef MEREMULATORDETAILSWIDGET_H
#define MEREMULATORDETAILSWIDGET_H

#include <QIcon>
#include <QPointer>
#include <QWidget>

namespace Sfdk {
class Emulator;
}

namespace Utils {
class PortList;
}

namespace Mer {
namespace Internal {

namespace Ui {
class MerEmulatorDetailsWidget;
}

class MerEmulatorDetailsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MerEmulatorDetailsWidget(QWidget *parent = 0);
    ~MerEmulatorDetailsWidget() override;

    QString searchKeyWordMatchString() const;
    void setEmulator(const Sfdk::Emulator *emulator);
    void setTestButtonEnabled(bool enabled);
    void setStatus(const QString &status);
    void setVmOffStatus(bool vmOff);
    void setFactorySnapshot(const QString &snapshotName);
    void setSshTimeout(int timeout);
    void setSshPort(quint16 port);
    void setQmlLivePorts(const Utils::PortList &ports);
    void setFreePorts(const Utils::PortList &ports);
    void setMemorySizeMb(int sizeMb);
    void setCpuCount(int count);
    void setVdiCapacityMb(int capacityMb);

signals:
    void factorySnapshotChanged(const QString &snapshotName);
    void testConnectionButtonClicked();
    void sshPortChanged(quint16 port);
    void sshTimeoutChanged(int timeout);
    void qmlLivePortsChanged(const Utils::PortList &ports);
    void freePortsChanged(const Utils::PortList &ports);
    void memorySizeMbChanged(int sizeMb);
    void cpuCountChanged(int count);
    void vdiCapacityMbChnaged(int value);

private:
    void selectFactorySnapshot();

private:
    Ui::MerEmulatorDetailsWidget *m_ui;
    QPointer<const Sfdk::Emulator> m_emulator;
};

} // Internal
} // Mer

#endif // MEREMULATORDETAILSWIDGET_H
