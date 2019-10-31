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

#ifndef MEREMULATOROPTIONSWIDGET_H
#define MEREMULATOROPTIONSWIDGET_H

#include <QMap>
#include <QUrl>
#include <QWidget>

#include <memory>
#include <vector>

namespace Sfdk {
class Emulator;
}

namespace Utils {
class PortList;
}

namespace Mer {
namespace Internal {

namespace Ui {
class MerEmulatorOptionsWidget;
}

class MerEmulatorOptionsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MerEmulatorOptionsWidget(QWidget *parent = 0);
    ~MerEmulatorOptionsWidget() override;

    QString searchKeyWordMatchString() const;
    void setEmulator(const QUrl &uri);
    void store();

signals:
    void updateSearchKeys();

private:
    bool lockDownConnectionsOrCancelChangesThatNeedIt(QList<Sfdk::Emulator *> *lockedDownEmulators);

private slots:
    void onEmulatorAdded(int index);
    void onAboutToRemoveEmulator(int index);
    void onEmulatorChanged(int index);
    void onAddButtonClicked();
    void onRemoveButtonClicked();
    void onTestConnectionButtonClicked();
    void onStartVirtualMachineButtonClicked();
    void onStopVirtualMachineButtonClicked();
    void onRegenerateSshKeysButtonClicked();
    void onEmulatorModeButtonClicked();
    void onFactoryResetButtonClicked();
    void update();
    void onVmOffChanged(bool vmOff);

private:
    Ui::MerEmulatorOptionsWidget *m_ui;
    QUrl m_virtualMachine;
    QMetaObject::Connection m_vmOffConnection;
    QString m_status;
    QMap<QUrl, Sfdk::Emulator *> m_emulators;
    std::vector<std::unique_ptr<Sfdk::Emulator>> m_newEmulators;
    QMap<Sfdk::Emulator *, QString> m_factorySnapshot;
    QMap<Sfdk::Emulator *, int> m_sshTimeout;
    QMap<Sfdk::Emulator *, quint16> m_sshPort;
    QMap<Sfdk::Emulator *, Utils::PortList> m_qmlLivePorts;
    QMap<Sfdk::Emulator *, Utils::PortList> m_freePorts;
    QMap<Sfdk::Emulator *, int> m_vdiCapacityMb;
    QMap<Sfdk::Emulator *, int> m_memorySizeMb;
    QMap<Sfdk::Emulator *, int> m_cpuCount;
};

} // Internal
} // Mer

#endif // MEREMULATOROPTIONSWIDGET_H
