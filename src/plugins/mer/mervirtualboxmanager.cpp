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

#include "mervirtualboxmanager.h"
#include "merconstants.h"

#include <utils/qtcassert.h>
#include <utils/hostosinfo.h>

#include <QDebug>
#include <QDir>
#include <QProcess>
#include <QSettings>

const char VBOXMANAGE[] = "VBoxManage";
const char LIST[] = "list";
const char RUNNINGVMS[] = "runningvms";
const char VMS[] = "vms";
const char SHOWVMINFO[] = "showvminfo";
const char SHOWHDINFO[] = "showhdinfo";
const char MACHINE_READABLE[] = "--machinereadable";
const char STARTVM[] = "startvm";
const char CONTROLVM[] = "controlvm";
const char MODIFYHD[] = "modifyhd";
const char RESIZE[] = "--resize";
const char ACPI_POWER_BUTTON[] = "acpipowerbutton";
const char TYPE[] = "--type";
const char HEADLESS[] = "headless";
const char SHAREDFOLDER[] = "sharedfolder";
const char SHARE_NAME[] = "--name";
const char REMOVE_SHARED[] = "remove";
const char HOSTPATH[] = "--hostpath";
const char ADD_SHARED[] = "add";

namespace Mer {
namespace Internal {

static VirtualMachineInfo virtualMachineInfoFromOutput(const QString &output);
static VirtualMachineDiskImageInfo virtualMachineDiskImageInfoFromOutput(const QString &output);
static QString getVBoxManageOutput(const QStringList& arguments, bool *ok);
static int getVirtualMachineDiskImageCapacity(const QString &uuid);
static bool isVirtualMachineListed(const QString &vmName, const QString &output);
static QStringList listedVirtualMachines(const QString &output);

static QString vBoxManagePath()
{
    if (Utils::HostOsInfo::isWindowsHost()) {
        static QString path;
        if (path.isEmpty()) {
            path = QString::fromLocal8Bit(qgetenv("VBOX_INSTALL_PATH"));
            if (path.isEmpty()) {
                // env var name for VirtualBox 4.3.12 changed to this
                path = QString::fromLocal8Bit(qgetenv("VBOX_MSI_INSTALL_PATH"));
                if (path.isEmpty()) {
                    // Not found in environment? Look up registry.
                    QSettings s(QLatin1String("HKEY_LOCAL_MACHINE\\SOFTWARE\\Oracle\\VirtualBox"),
                                QSettings::NativeFormat);
                    path = s.value(QLatin1String("InstallDir")).toString();
                    if (path.startsWith(QLatin1Char('"')) && path.endsWith(QLatin1Char('"')))
                        path = path.mid(1, path.length() - 2); // remove quotes
                }
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

MerVirtualBoxManager *MerVirtualBoxManager::m_instance = 0;

MerVirtualBoxManager::MerVirtualBoxManager(QObject *parent):
    QObject(parent)
{
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

bool MerVirtualBoxManager::updateSharedFolder(const QString &vmName, const QString &mountName, const QString &newFolder)
{
    if (isVirtualMachineRunning(vmName)) {
        qWarning() << "Virtual machine " << vmName << " is running, unable to update shared folder.";
        return false;
    }

    QStringList rargs;
    rargs.append(QLatin1String(SHAREDFOLDER));
    rargs.append(QLatin1String(REMOVE_SHARED));
    rargs.append(vmName);
    rargs.append(QLatin1String(SHARE_NAME));
    rargs.append(mountName);
    QProcess rproc;
    rproc.start(vBoxManagePath(), rargs);
    if (!rproc.waitForFinished()) {
        qWarning() << "VBoxManage failed to remove " << mountName;
        return false;
    }

    QStringList aargs;
    aargs.append(QLatin1String(SHAREDFOLDER));
    aargs.append(QLatin1String(ADD_SHARED));
    aargs.append(vmName);
    aargs.append(QLatin1String(SHARE_NAME));
    aargs.append(mountName);
    aargs.append(QLatin1String(HOSTPATH));
    aargs.append(newFolder);

    QProcess aproc;
    aproc.start(vBoxManagePath(), aargs);
    if (!aproc.waitForFinished()) {
        qWarning() << "VBoxManage failed to add " << mountName;
        return false;
    }

    return true;
}

bool MerVirtualBoxManager::resizeDiskImage(const QString &vmName, const QString &uuid, int capacity)
{
    if (isVirtualMachineRunning(vmName)) {
        qWarning() << "Virtual machine " << vmName << " is running, unable to resize disk image.";
        return false;
    }

    QStringList args;
    args.append(QLatin1String(MODIFYHD));
    args.append(uuid);
    args.append(QLatin1String(RESIZE));
    args.append(QString::number(capacity));

    QProcess proc;
    proc.start(vBoxManagePath(), args);
    if (!proc.waitForFinished()) {
        qWarning() << "VBoxManage failed to resize disk image with UUID " << uuid;
        return false;
    }

    return true;
}

VirtualMachineInfo MerVirtualBoxManager::fetchVirtualMachineInfo(const QString &vmName)
{
    VirtualMachineInfo info;
    QStringList arguments;
    arguments.append(QLatin1String(SHOWVMINFO));
    arguments.append(vmName);
    arguments.append(QLatin1String(MACHINE_READABLE));

    bool ok = false;
    const QString output = getVBoxManageOutput(arguments, &ok);

    if (!ok)
        return info;

    return virtualMachineInfoFromOutput(output);
}

VirtualMachineDiskImageInfo MerVirtualBoxManager::fetchVirtualMachineDiskImageInfo(const QString &vmName)
{
    VirtualMachineDiskImageInfo info;
    QStringList arguments;
    arguments.append(QLatin1String(SHOWVMINFO));
    arguments.append(vmName);
    arguments.append(QLatin1String(MACHINE_READABLE));

    bool ok = false;
    const QString output = getVBoxManageOutput(arguments, &ok);

    if (!ok)
        return info;

    return virtualMachineDiskImageInfoFromOutput(output);
}

void MerVirtualBoxManager::startVirtualMachine(const QString &vmName,bool headless)
{
    // Start VM if it is not running
    if (isVirtualMachineRunning(vmName))
        return;

    if (!isVirtualMachineRegistered(vmName)) {
        qWarning() << "MerVirtualBoxManager: VM not registered:" << vmName;
        return;
    }

    QStringList arguments;
    arguments.append(QLatin1String(STARTVM));
    arguments.append(vmName);
    if (headless) {
        arguments.append(QLatin1String(TYPE));
        arguments.append(QLatin1String(HEADLESS));
    }

    QProcess *process = new QProcess(instance());
    connect(process, SIGNAL(error(QProcess::ProcessError)), process, SLOT(deleteLater()));
    connect(process, SIGNAL(finished(int,QProcess::ExitStatus)), process, SLOT(deleteLater()));
    process->start(vBoxManagePath(), arguments);
}

void MerVirtualBoxManager::shutVirtualMachine(const QString &vmName)
{
    if (!isVirtualMachineRunning(vmName))
        return;

    QStringList arguments;
    arguments.append(QLatin1String(CONTROLVM));
    arguments.append(vmName);
    arguments.append(QLatin1String(ACPI_POWER_BUTTON));

    QProcess *process = new QProcess(instance());
    connect(process, SIGNAL(error(QProcess::ProcessError)), process, SLOT(deleteLater()));
    connect(process, SIGNAL(finished(int,QProcess::ExitStatus)), process, SLOT(deleteLater()));
    process->start(vBoxManagePath(), arguments);
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
    info.sshPort = 0;

    // Get ssh port, shared home and shared targets
    // 1 Name, 2 Protocol, 3 Host IP, 4 Host Port, 5 Guest IP, 6 Guest Port, 7 Shared Folder Name,
    // 8 Shared Folder Path, 9 MAC Addresses, 10 Session Type
    QRegExp rexp(QLatin1String("(?:Forwarding\\(\\d+\\)=\"(\\w+),(\\w+),(.*),(\\d+),(.*),(\\d+)\")"
                               "|(?:SharedFolderNameMachineMapping\\d+=\"(\\w+)\"\\W*"
                               "SharedFolderPathMachineMapping\\d+=\"(.*)\")"
                               "|(?:macaddress\\d+=\"(.*)\")"
                               "|(?:SessionType=\"(.*)\")"));

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
        } else if(rexp.cap(0).startsWith(QLatin1String("SharedFolderNameMachineMapping"))) {
            if (rexp.cap(7) == QLatin1String("home"))
                info.sharedHome = rexp.cap(8);
            else if (rexp.cap(7) == QLatin1String("targets"))
                info.sharedTargets = rexp.cap(8);
            else if (rexp.cap(7) == QLatin1String("ssh"))
                info.sharedSsh = rexp.cap(8);
            else if (rexp.cap(7) == QLatin1String("config"))
                info.sharedConfig = rexp.cap(8);
            else if (rexp.cap(7).startsWith(QLatin1String("src")))
                info.sharedSrc = rexp.cap(8);
        } else if(rexp.cap(0).startsWith(QLatin1String("macaddress"))) {
            QRegExp rx(QLatin1String("(?:([0-9A-F]{2})([0-9A-F]{2})([0-9A-F]{2})([0-9A-F]{2})([0-9A-F]{2})([0-9A-F]{2}))"));
            QString mac = rexp.cap(9);
            QStringList macFields;
            if(rx.exactMatch(mac)) {
                macFields = rx.capturedTexts();
            }
            if(!macFields.isEmpty()) {
                macFields.removeFirst();
                info.macs << macFields.join(QLatin1String(":"));
            }
        } else if (rexp.cap(0).startsWith(QLatin1String("SessionType"))) {
            info.headless = rexp.cap(10) == QLatin1String("headless");
        }
    }

    return info;
}

VirtualMachineDiskImageInfo virtualMachineDiskImageInfoFromOutput(const QString &output)
{
    VirtualMachineDiskImageInfo info;

    // Look for UUID of the disk in the smallest SATA controller instance.
    // Controllers with disks are named "SATA-ImageUUID-0-0", "SATA-ImageUUID-1-0",
    // and so on. Controllers with no disks won't appear in the output, since they
    // have no UUIDs associated with them. Each SATA controller has one device, so
    // the port is always "0".
    //
    // The captured subexpressions are:
    //   1 = SATA controller instance (\\d+)
    //   2 = Disk image UUID (.*).
    QRegExp rexp(QLatin1String("(?:\"SATA-ImageUUID-(\\d+)-0\"=\"(.*)\")"));
    rexp.setMinimal(true);

    int pos = 0;
    int controller = -1;
    QString uuid;

    while ((pos = rexp.indexIn(output, pos)) != -1) {
        pos += rexp.matchedLength();
        if ((controller < 0) || (controller > rexp.cap(1).toInt())) {
            controller = rexp.cap(1).toInt();
            uuid = rexp.cap(2);
        }
    }

    // If a UUID was found, fetch the capacity of the disk image.
    if (controller >= 0) {
        info.uuid = uuid;
        info.capacity = getVirtualMachineDiskImageCapacity(uuid);
    }

    return info;
}

QString getVBoxManageOutput(const QStringList& arguments, bool *ok)
{
    QString output;
    QProcess process;
    process.start(QLatin1String(VBOXMANAGE), arguments);
    const bool processOk = process.waitForFinished();

    if (processOk)
        output = QString::fromLocal8Bit(process.readAllStandardOutput());

    if (ok)
        *ok = processOk;

    return output;
}

int getVirtualMachineDiskImageCapacity(const QString& uuid)
{
    int capacity = 0;

    if (!uuid.isEmpty()) {
        QStringList arguments;
        arguments.append(QLatin1String(SHOWHDINFO));
        arguments.append(uuid);

        bool ok = false;
        const QString output = getVBoxManageOutput(arguments, &ok);

        if (ok) {
            // Get disk image file capacity in megabytes.
            QRegExp rexp(QLatin1String("(?:Capacity:\\s*(\\d+)\\s*MBytes)"));

            if (rexp.indexIn(output) != -1) {
                capacity = rexp.cap(1).toInt();
            }
        }
    }

    return capacity;
}

} // Internal
} // VirtualBox
