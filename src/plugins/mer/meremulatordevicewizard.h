#ifndef MEREMULATORDEVICEWIZARD_H
#define MEREMULATORDEVICEWIZARD_H

#include <projectexplorer/devicesupport/idevice.h>
#include "meremulatordevicewizardpages.h"
#include <QWizard>

namespace Mer {
namespace Internal {

class MerEmulatorDeviceWizard : public QWizard
{
    Q_OBJECT
public:
    MerEmulatorDeviceWizard(QWidget *parent = 0);

    int index() const;
    QString configName() const;
    QString userName() const;
    QString rootName() const;
    int sshPort() const;
    int timeout() const;
    QString userPrivateKey() const;
    QString rootPrivateKey() const;
    QString emulatorVm() const;
    QString freePorts() const;
    QString sharedConfigPath() const;
    QString sharedSshPath() const;

    bool isUserNewSshKeysRquired() const;
    bool isRootNewSshKeysRquired() const;

private:
    MerEmualtorVMPage m_vmPage;
    MerEmualtorSshPage m_sshPage;
    int m_index;
};

}
}

#endif // MEREMULATORDEVICEWIZARD_H
