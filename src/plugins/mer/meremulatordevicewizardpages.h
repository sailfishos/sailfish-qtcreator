/****************************************************************************
**
** Copyright (C) 2012-2015,2018-2019 Jolla Ltd.
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

#ifndef MEREMULATORDEVICEWIZARDPAGES_H
#define MEREMULATORDEVICEWIZARDPAGES_H

#include <QWizardPage>

namespace Mer {
namespace Internal {

namespace Ui {
    class MerEmulatorDeviceWizardVMPage;
    class MerEmulatorDeviceWizardSshPage;
}

class MerEmualtorVMPage : public QWizardPage
{
    Q_OBJECT
public:
    MerEmualtorVMPage(QWidget *parent = 0);
    ~MerEmualtorVMPage();

    QString configName() const;
    int sshPort() const;
    int timeout() const;
    QString emulatorVm() const;
    QString factorySnapshot() const { return m_factorySnapshot; }
    QString freePorts() const;
    QString qmlLivePorts() const;
    QString sharedConfigPath() const;
    QString sharedSshPath() const;
    QString mac() const;
    int memorySizeMb() const;
    int cpuCount() const;
    int vdiCapacityMb() const;
    bool isComplete() const override;
private slots:
    void handleEmulatorVmChanged(const QString &vmName);
private:
     Ui::MerEmulatorDeviceWizardVMPage *m_ui;
     QString m_factorySnapshot;
};

class MerEmualtorSshPage : public QWizardPage
{
    Q_OBJECT
public:
    MerEmualtorSshPage(QWidget *parent = 0);
    ~MerEmualtorSshPage();

    void initializePage() override;

    QString userName() const;
    QString userPrivateKey() const;
    QString rootName() const;
    QString rootPrivateKey() const;
    bool isUserNewSshKeysRquired() const;
    bool isRootNewSshKeysRquired() const;

private:
     Ui::MerEmulatorDeviceWizardSshPage *m_ui;
};

}
}
#endif
