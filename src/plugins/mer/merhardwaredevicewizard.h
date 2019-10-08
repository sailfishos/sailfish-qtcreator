/****************************************************************************
**
** Copyright (C) 2012-2015 Jolla Ltd.
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

#ifndef MERHARDWAREDEVICEWIZARD_H
#define MERHARDWAREDEVICEWIZARD_H

#include "merhardwaredevicewizardpages.h"

#include <projectexplorer/abi.h>
#include <remotelinux/genericlinuxdeviceconfigurationwizardpages.h>

#include <QWizard>

namespace QSsh {
    class SshConnectionParameters;
}

namespace Mer {
namespace Internal {

class MerHardwareDeviceWizard : public QWizard
{
    Q_OBJECT
public:
    explicit MerHardwareDeviceWizard(QWidget *parent = 0);
    ~MerHardwareDeviceWizard() override;
    QString hostName() const;
    QString userName() const;
    ProjectExplorer::Abi::Architecture architecture() const;
    QString deviceName() const;
    QString privateKeyFilePath() const;
    QString publicKeyFilePath() const;
    QString configurationName() const;
    int sshPort() const;
    int timeout() const;
    QString freePorts() const;
    bool isNewSshKeysRquired() const;
    QString sharedSshPath() const;


private:
    MerHardwareDeviceWizardSelectionPage m_selectionPage;
    MerHardwareDeviceWizardSetupPage m_setupPage;
    RemoteLinux::GenericLinuxDeviceConfigurationWizardFinalPage m_finalPage;
};

} // Internal
} // Mer

#endif
