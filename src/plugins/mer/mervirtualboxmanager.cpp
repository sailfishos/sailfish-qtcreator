/****************************************************************************
**
** Copyright (C) 2012 - 2013 Jolla Ltd.
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

#include "mervirtualboxmanager.h"
#include "merconstants.h"

#include <utils/qtcassert.h>
#include <utils/hostosinfo.h>
#include <projectexplorer/taskhub.h>
#include <projectexplorer/projectexplorer.h>

#include <QDir>
#include <QProcess>
#include <QSettings>
#include <QMessageBox>

const char VBOXMANAGE[] = "VBoxManage";
const char LIST[] = "list";
const char RUNNINGVMS[] = "runningvms";
const char VMS[] = "vms";
const char SHOWVMINFO[] = "showvminfo";
const char MACHINE_READABLE[] = "--machinereadable";
const char STARTVM[] = "startvm";
const char CONTROLVM[] = "controlvm";
const char ACPI_POWER_BUTTON[] = "acpipowerbutton";

using namespace ProjectExplorer;

namespace Mer {
namespace Internal {

static VirtualMachineInfo virtualMachineInfoFromOutput(const QString &output);
static bool isVirtualMachineListed(const QString &vmName, const QString &output);
static QStringList listedVirtualMachines(const QString &output);

static QString vBoxManagePath()
{
    if (Utils::HostOsInfo::isWindowsHost()) {
        static QString path;
        if (path.isEmpty()) {
            path = QString::fromLocal8Bit(qgetenv("VBOX_INSTALL_PATH"));
            if (path.isEmpty()) {
                // Not found in environment? Look up registry.
                QSettings s(QLatin1String("HKEY_LOCAL_MACHINE\\SOFTWARE\\Oracle\\VirtualBox"),
                            QSettings::NativeFormat);
                path = s.value(QLatin1String("InstallDir")).toString();
                if (path.startsWith(QLatin1Char('"')) && path.endsWith(QLatin1Char('"')))
                    path = path.mid(1, path.length() - 2); // remove quotes
            }

            if (!path.isEmpty())
                path.append(QDir::separator() + QLatin1String(VBOXMANAGE));
        }
        QTC_ASSERT(!path.isEmpty(), return path);
        return path;
    } else {
        return QLatin1String(VBOXMANAGE);
    }
}

MerVirtualBoxManager::MerVirtualBoxManager(QObject *parent):
    QObject(parent)
{
    TaskHub *th = ProjectExplorerPlugin::instance()->taskHub();
    th->addCategory(Core::Id(Constants::MER_TASKHUB_CATEGORY), tr("Virtual Machine Error"));
    connect(th, SIGNAL(taskAdded(ProjectExplorer::Task)),
            SLOT(handleTaskAdded(ProjectExplorer::Task)));
    m_instance = this;
}

MerVirtualBoxManager::~MerVirtualBoxManager()
{
    m_instance = 0;
}

MerVirtualBoxManager *MerVirtualBoxManager::instance()
{
    QTC_CHECK(m_instance);
    return m_instance;
}

bool MerVirtualBoxManager::isVirtualMachineRunning(const QString &vmName)
{
    QStringList arguments;
    arguments.append(QLatin1String(LIST));
    arguments.append(QLatin1String(RUNNINGVMS));
    QProcess process;
    process.start(vBoxManagePath(), arguments);
    if (!process.waitForFinished())
        return false;

    return isVirtualMachineListed(vmName, QString::fromLocal8Bit(process.readAllStandardOutput()));
}

bool MerVirtualBoxManager::isVirtualMachineRegistered(const QString &vmName)
{
    QStringList arguments;
    arguments.append(QLatin1String(LIST));
    arguments.append(QLatin1String(VMS));
    QProcess process;
    process.start(vBoxManagePath(), arguments);
    if (!process.waitForFinished())
        return false;

    return isVirtualMachineListed(vmName, QString::fromLocal8Bit(process.readAllStandardOutput()));
}

VirtualMachineInfo MerVirtualBoxManager::fetchVirtualMachineInfo(const QString &vmName)
{
    VirtualMachineInfo info;
    QStringList arguments;
    arguments.append(QLatin1String(SHOWVMINFO));
    arguments.append(vmName);
    arguments.append(QLatin1String(MACHINE_READABLE));
    QProcess process;
    process.start(vBoxManagePath(), arguments);
    if (!process.waitForFinished())
        return info;

    return virtualMachineInfoFromOutput(QString::fromLocal8Bit(process.readAllStandardOutput()));
}

bool MerVirtualBoxManager::startVirtualMachine(const QString &vmName)
{
    // Start VM if it is not running
    if (isVirtualMachineRunning(vmName))
        return true;
    QStringList arguments;
    arguments.append(QLatin1String(STARTVM));
    arguments.append(vmName);
    return QProcess::execute(vBoxManagePath(), arguments);
}

bool MerVirtualBoxManager::shutVirtualMachine(const QString &vmName)
{
    if (!isVirtualMachineRunning(vmName))
        return true;
    QStringList arguments;
    arguments.append(QLatin1String(CONTROLVM));
    arguments.append(vmName);
    arguments.append(QLatin1String(ACPI_POWER_BUTTON));
    QProcess process;
    process.start(vBoxManagePath(), arguments);
    if (!process.waitForFinished())
        return false;

    return isVirtualMachineRunning(vmName);
}

QStringList MerVirtualBoxManager::fetchRegisteredVirtualMachines()
{
    QStringList vms;
    QStringList arguments;
    arguments.append(QLatin1String(LIST));
    arguments.append(QLatin1String(VMS));
    QProcess process;
    process.start(vBoxManagePath(), arguments);
    if (!process.waitForFinished())
        return vms;

    return listedVirtualMachines(QString::fromLocal8Bit(process.readAllStandardOutput()));
}

bool isVirtualMachineListed(const QString &vmName, const QString &output)
{
    return output.indexOf(QRegExp(QString::fromLatin1("\\B\"%1\"\\B").arg(vmName))) != -1;
}

QStringList listedVirtualMachines(const QString &output)
{
    QStringList vms;
    QRegExp rx(QLatin1String("\"(.*)\""));
    rx.setMinimal(true);
    int pos = 0;
    while ((pos = rx.indexIn(output, pos)) != -1) {
        pos += rx.matchedLength();
        vms.append(rx.cap(1));
    }
    return vms;
}

VirtualMachineInfo virtualMachineInfoFromOutput(const QString &output)
{
    VirtualMachineInfo info;
    info.sshPort = -1;

    // Get ssh port, shared home and shared targets
    // 1 Name, 2 Protocol, 3 Host IP, 4 Host Port, 5 Guest IP, 6 Guest Port, 7 Shared Folder Name,
    // 8 Shared Folder Path
    QRegExp rexp(QLatin1String("(?:Forwarding\\(\\d+\\)=\"(\\w+),(\\w+),(.*),(\\d+),(.*),(\\d+)\")"
                               "|(?:SharedFolderNameMachineMapping\\d+=\"(\\w+)\"\\W*"
                               "SharedFolderPathMachineMapping\\d+=\"(.*)\")"));
    rexp.setMinimal(true);
    int pos = 0;
    while ((pos = rexp.indexIn(output, pos)) != -1) {
        pos += rexp.matchedLength();
        if (rexp.cap(0).startsWith(QLatin1String("Forwarding"))) {
            quint16 port = rexp.cap(4).toUInt();
            if (rexp.cap(1).contains(QLatin1String("ssh")))
                info.sshPort = port;
            else if (rexp.cap(1).contains(QLatin1String("www")))
                info.wwwPort = port;
            else
                info.freePorts << port;
        } else {
            if (rexp.cap(7) == QLatin1String("home"))
                info.sharedHome = rexp.cap(8);
            else if (rexp.cap(7) == QLatin1String("targets"))
                info.sharedTargets = rexp.cap(8);
            else if (rexp.cap(7) == QLatin1String("ssh"))
                info.sharedSsh = rexp.cap(8);
        }
    }

    return info;
}

void MerVirtualBoxManager::handleTaskAdded(const Task &task)
{
    static QRegExp regExp(QLatin1String("Virtual Machine '(.*)' is not running!"));
    if (regExp.indexIn(task.description) != -1) {
        QString vm = regExp.cap(1);
        if (promptToStart(vm)) {
            QStringList vmList = MerVirtualBoxManager::fetchRegisteredVirtualMachines();
            if (vmList.contains(vm))
                MerVirtualBoxManager::startVirtualMachine(vm);
        }
    }
}

bool MerVirtualBoxManager::promptToStart(const QString& vm) const
{
    const QMessageBox::StandardButton response =
        QMessageBox::question(0, tr("Start Virtual Machine"),
                              tr("Virtual Machine '%1' is not running! Please start the "
                                 "Virtual Machine and retry after the Virtual Machine is "
                                 "running.\n\n"
                                 "Start Virtual Machine now?").arg(vm),
                              QMessageBox::No | QMessageBox::Yes, QMessageBox::Yes);
    return (response == QMessageBox::Yes);
}

} // Internal
} // VirtualBox
