#ifndef MEREMULATORDEVICEWIZARD_H
#define MEREMULATORDEVICEWIZARD_H

#include "meremulatordevicewizardpages.h"
#include <QWizard>

namespace Mer {
namespace Internal {

class MerEmulatorDeviceWizard : public QWizard
{
    Q_OBJECT
public:
    explicit MerEmulatorDeviceWizard(QWidget *parent = 0);

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
    QString mac() const;

    bool isUserNewSshKeysRquired() const;
    bool isRootNewSshKeysRquired() const;

private:
    MerEmualtorVMPage m_vmPage;
    MerEmualtorSshPage m_sshPage;
};

}
}

#endif // MEREMULATORDEVICEWIZARD_H
