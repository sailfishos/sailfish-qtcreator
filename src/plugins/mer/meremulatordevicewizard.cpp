/****************************************************************************
**
** Copyright (C) 2013-2015,2017-2019 Jolla Ltd.
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

#include "meremulatordevicewizard.h"

#include <sfdk/virtualmachine_p.h>
#include <sfdk/vboxvirtualmachine_p.h>

#include <coreplugin/id.h>
#include <utils/qtcassert.h>

#include "mersdkmanager.h"

using namespace Sfdk;

namespace Mer {
namespace Internal {

MerEmulatorDeviceWizard::MerEmulatorDeviceWizard(QWidget *parent): QWizard(parent)
{
    setWindowTitle(tr("New Sailfish OS Emulator Device Setup"));
    addPage(&m_vmPage);
    addPage(&m_sshPage);
    m_sshPage.setCommitPage(true);
}

Core::Id MerEmulatorDeviceWizard::emulatorId() const
{
    const QUrl uri = m_vmPage.emulatorVm();
    // We know we do not have other than VirtualBox based emulators
    QTC_CHECK(VirtualMachineFactory::typeFromUri(uri) == VBoxVirtualMachine::staticType());
    return Core::Id::fromString(VirtualMachineFactory::nameFromUri(uri));
}

QString MerEmulatorDeviceWizard::configName() const
{
    return m_vmPage.configName();
}

QString MerEmulatorDeviceWizard::userName() const
{
    return m_sshPage.userName();
}

QString MerEmulatorDeviceWizard::rootName() const
{
    return m_sshPage.rootName();
}

int MerEmulatorDeviceWizard::sshPort() const
{
    return m_vmPage.sshPort();
}

int MerEmulatorDeviceWizard::timeout() const
{
    return m_vmPage.timeout();
}

QString MerEmulatorDeviceWizard::userPrivateKey() const
{
    return m_sshPage.userPrivateKey();
}

QString MerEmulatorDeviceWizard::rootPrivateKey() const
{
    return m_sshPage.rootPrivateKey();
}

QUrl MerEmulatorDeviceWizard::emulatorVm() const
{
    return m_vmPage.emulatorVm();
}

QString MerEmulatorDeviceWizard::factorySnapshot() const
{
    return m_vmPage.factorySnapshot();
}

QString MerEmulatorDeviceWizard::freePorts() const
{
     return m_vmPage.freePorts();
}

QString MerEmulatorDeviceWizard::qmlLivePorts() const
{
     return m_vmPage.qmlLivePorts();
}

QString MerEmulatorDeviceWizard::sharedConfigPath() const
{
     return m_vmPage.sharedConfigPath();
}

QString MerEmulatorDeviceWizard::sharedSshPath() const
{
    return m_vmPage.sharedSshPath();
}

bool MerEmulatorDeviceWizard::isUserNewSshKeysRquired() const
{
    return m_sshPage.isUserNewSshKeysRquired();
}

bool MerEmulatorDeviceWizard::isRootNewSshKeysRquired() const
{
    return m_sshPage.isRootNewSshKeysRquired();
}

QString MerEmulatorDeviceWizard::mac() const
{
    return m_vmPage.mac();
}

int MerEmulatorDeviceWizard::memorySizeMb() const
{
    return m_vmPage.memorySizeMb();
}

int MerEmulatorDeviceWizard::cpuCount() const
{
    return m_vmPage.cpuCount();
}

int MerEmulatorDeviceWizard::vdiCapacityMb() const
{
    return m_vmPage.vdiCapacityMb();
}

}
}

