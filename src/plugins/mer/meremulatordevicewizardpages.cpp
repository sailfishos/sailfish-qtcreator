/****************************************************************************
**
** Copyright (C) 2012 - 2014 Jolla Ltd.
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

#include "meremulatordevicewizardpages.h"
#include "ui_meremulatordevicewizardvmpage.h"
#include "ui_meremulatordevicewizardsshpage.h"

#include "merconnection.h"
#include "merconstants.h"
#include "meremulatordevicewizard.h"
#include "mervirtualboxmanager.h"

#include <projectexplorer/devicesupport/devicemanager.h>
#include <utils/qtcassert.h>

#include <QDir>

using namespace ProjectExplorer;

namespace Mer {
namespace Internal {



QString trimNone(const QString& text)
{
        return text != QObject::tr("none") ? text : QString();
}

MerEmualtorVMPage::MerEmualtorVMPage(QWidget *parent): QWizardPage(parent),
    m_ui(new Ui::MerEmulatorDeviceWizardVMPage)
{
    m_ui->setupUi(this);

    m_ui->timeoutSpinBox->setMinimum(1);
    m_ui->timeoutSpinBox->setMaximum(65535);
    m_ui->timeoutSpinBox->setValue(30);

    static QRegExp regExp(tr("Emulator"));

    const QSet<QString> usedVMs = MerConnection::usedVirtualMachines().toSet();
    const QStringList registeredVMs = MerVirtualBoxManager::fetchRegisteredVirtualMachines();
    foreach (const QString &vm, registeredVMs) {
        // add only unused machines
        if (!usedVMs.contains(vm)) {
            m_ui->emulatorComboBox->addItem(vm);
            if (regExp.indexIn(vm) != -1) {
                //preselect emulator
                m_ui->emulatorComboBox->setCurrentIndex(m_ui->emulatorComboBox->count()-1);
            }
        }
    }
    connect(m_ui->emulatorComboBox,
            static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::currentIndexChanged),
            this,
            &MerEmualtorVMPage::handleEmulatorVmChanged);
    connect(m_ui->configNameLineEdit, &QLineEdit::textChanged,
            this, &MerEmualtorVMPage::completeChanged);
    handleEmulatorVmChanged(m_ui->emulatorComboBox->currentText());
}

MerEmualtorVMPage::~MerEmualtorVMPage()
{

}

QString MerEmualtorVMPage::configName() const
{
    return m_ui->configNameLineEdit->text();
}

int MerEmualtorVMPage::sshPort() const
{
    return m_ui->sshPortLabelEdit->text().toInt();
}

int MerEmualtorVMPage::timeout() const
{
    return m_ui->timeoutSpinBox->value();
}

QString MerEmualtorVMPage::emulatorVm() const
{
    return m_ui->emulatorComboBox->currentText();
}

QString MerEmualtorVMPage::freePorts() const
{
    return m_ui->portsLineEdit->text();
}

QString MerEmualtorVMPage::qmlLivePorts() const
{
    return m_ui->qmlLivePortsLabelEdit->text();
}

QString MerEmualtorVMPage::sharedConfigPath() const
{
    return trimNone(m_ui->configFolderLabelEdit->text());
}

QString MerEmualtorVMPage::sharedSshPath() const
{
    return trimNone(m_ui->sshFolderLabelEdit->text());
}

QString MerEmualtorVMPage::mac() const
{
    return m_ui->macLabelEdit->text();
}

void MerEmualtorVMPage::handleEmulatorVmChanged(const QString &vmName)
{
    int i = 1;
    QString tryName = vmName;
    while (DeviceManager::instance()->hasDevice(tryName))
        tryName = vmName + QString::number(++i);
    m_ui->configNameLineEdit->setText(tryName);

    VirtualMachineInfo info = MerVirtualBoxManager::fetchVirtualMachineInfo(vmName);
    if (info.sshPort == 0)
        m_ui->sshPortLabelEdit->setText(tr("none"));
    else
        m_ui->sshPortLabelEdit->setText(QString::number(info.sshPort));

    QStringList freePorts;
    foreach (quint16 port, info.freePorts)
        freePorts << QString::number(port);
    m_ui->portsLineEdit->setText(freePorts.join(QLatin1Char(',')));

    QStringList qmlLivePorts;
    foreach (quint16 port, info.qmlLivePorts)
        qmlLivePorts << QString::number(port);
    m_ui->qmlLivePortsLabelEdit->setText(qmlLivePorts.join(QLatin1Char(',')));

    QString configFolder(QDir::toNativeSeparators(info.sharedConfig));
    QString sshFolder(QDir::toNativeSeparators(info.sharedSsh));

    if (!configFolder.isEmpty())
        m_ui->configFolderLabelEdit->setText(configFolder);
    else
        m_ui->configFolderLabelEdit->setText(tr("none"));

    if (!sshFolder.isEmpty())
        m_ui->sshFolderLabelEdit->setText(sshFolder);
    else
        m_ui->sshFolderLabelEdit->setText(tr("none"));

    if (info.macs.count()>1)
        m_ui->macLabelEdit->setText(info.macs.at(1));
    else
        m_ui->macLabelEdit->setText(tr("none"));

}


bool MerEmualtorVMPage::isComplete() const
{
    return !configName().isEmpty()
            && !DeviceManager::instance()->hasDevice(configName())
            && !emulatorVm().isEmpty();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

MerEmualtorSshPage::MerEmualtorSshPage(QWidget *parent): QWizardPage(parent),
    m_ui(new Ui::MerEmulatorDeviceWizardSshPage)
{
    m_ui->setupUi(this);
    m_ui->userLineEdit->setText(QLatin1String(Constants::MER_DEVICE_DEFAULTUSER));
    m_ui->rootLineEdit->setText(QLatin1String(Constants::MER_DEVICE_ROOTUSER));
    //block "nemo" user
    m_ui->userLineEdit->setEnabled(false);
    //block "root" user
    m_ui->rootLineEdit->setEnabled(false);
}

MerEmualtorSshPage::~MerEmualtorSshPage()
{

}

void MerEmualtorSshPage::initializePage()
{
   QString index(QLatin1String("/ssh/private_keys/%1/"));
   const MerEmulatorDeviceWizard* wizard = qobject_cast<MerEmulatorDeviceWizard*>(this->wizard());
   QTC_ASSERT(wizard, return);
   //TODO: fix me
   QString sshKeyPath(QDir::toNativeSeparators(wizard->sharedConfigPath() +
                      index.arg(wizard->emulatorId().toString()).replace(QLatin1Char(' '), QLatin1Char('_')) +
                      QLatin1String("%1")));
   if (!wizard->sharedConfigPath().isEmpty()) {
       m_ui->userSshKeyLabelEdit->setText(sshKeyPath.arg(userName()));
       m_ui->rootSshKeyLabelEdit->setText(sshKeyPath.arg(rootName()));
       m_ui->userSshCheckBox->setChecked(true);
       m_ui->rootSshCheckBox->setChecked(true);
       m_ui->userSshCheckBox->setEnabled(true);
       m_ui->rootSshCheckBox->setEnabled(true);
   } else {
       m_ui->userSshKeyLabelEdit->setText(tr("none"));
       m_ui->rootSshKeyLabelEdit->setText(tr("none"));
       m_ui->userSshCheckBox->setChecked(false);
       m_ui->rootSshCheckBox->setChecked(false);
       m_ui->userSshCheckBox->setEnabled(false);
       m_ui->rootSshCheckBox->setEnabled(false);
   }

   QString sshDirectoryPath(QDir::toNativeSeparators(wizard->sharedSshPath() +
                            QLatin1String("/%1/") + QLatin1String(Constants::MER_AUTHORIZEDKEYS_FOLDER)));

   if (!wizard->sharedSshPath().isEmpty()) {
       m_ui->userAuthorizedFolderLabelEdit->setText(sshDirectoryPath.arg(userName()));
       m_ui->rootAuthorizedFolderLabelEdit->setText(sshDirectoryPath.arg(rootName()));
   } else {
       m_ui->userAuthorizedFolderLabelEdit->setText(tr("none"));
       m_ui->rootAuthorizedFolderLabelEdit->setText(tr("none"));
   }

}

QString MerEmualtorSshPage::userName() const
{
    return m_ui->userLineEdit->text();
}

QString MerEmualtorSshPage::userPrivateKey() const
{
    return trimNone(m_ui->userSshKeyLabelEdit->text());
}

QString MerEmualtorSshPage::rootName() const
{
    return m_ui->rootLineEdit->text();
}

QString MerEmualtorSshPage::rootPrivateKey() const
{
    return trimNone(m_ui->rootSshKeyLabelEdit->text());
}

bool MerEmualtorSshPage::isUserNewSshKeysRquired() const
{
    return m_ui->userSshCheckBox->isChecked();
}

bool MerEmualtorSshPage::isRootNewSshKeysRquired() const
{
    return m_ui->rootSshCheckBox->isChecked();
}

}
}
