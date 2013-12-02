#include "meremulatordevicewizard.h"
#include "mersdkmanager.h"

namespace Mer {
namespace Internal {

MerEmulatorDeviceWizard::MerEmulatorDeviceWizard(QWidget *parent): QWizard(parent)
{
    setWindowTitle(tr("New Mer Emulator Device  Setup"));
    addPage(&m_vmPage);
    addPage(&m_sshPage);
    m_sshPage.setCommitPage(true);
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

QString MerEmulatorDeviceWizard::emulatorVm() const
{
    return m_vmPage.emulatorVm();
}

QString MerEmulatorDeviceWizard::freePorts() const
{
     return m_vmPage.freePorts();
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

}
}

