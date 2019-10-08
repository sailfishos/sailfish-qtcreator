/****************************************************************************
**
** Copyright (C) 2013-2015,2018-2019 Jolla Ltd.
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

#ifndef MEREMULATORDEVICEWIZARD_H
#define MEREMULATORDEVICEWIZARD_H

#include "meremulatordevicewizardpages.h"

#include <QWizard>

namespace Core {
    class Id;
}

namespace Mer {
namespace Internal {

class MerEmulatorDeviceWizard : public QWizard
{
    Q_OBJECT
public:
    explicit MerEmulatorDeviceWizard(QWidget *parent = 0);

    Core::Id emulatorId() const;
    QString configName() const;
    QString userName() const;
    QString rootName() const;
    int sshPort() const;
    int timeout() const;
    QString userPrivateKey() const;
    QString rootPrivateKey() const;
    QString emulatorVm() const;
    QString factorySnapshot() const;
    QString freePorts() const;
    QString qmlLivePorts() const;
    QString sharedConfigPath() const;
    QString sharedSshPath() const;
    QString mac() const;
    int memorySizeMb() const;
    int cpuCount() const;
    int vdiCapacityMb() const;

    bool isUserNewSshKeysRquired() const;
    bool isRootNewSshKeysRquired() const;

private:
    MerEmualtorVMPage m_vmPage;
    MerEmualtorSshPage m_sshPage;
};

}
}

#endif // MEREMULATORDEVICEWIZARD_H
