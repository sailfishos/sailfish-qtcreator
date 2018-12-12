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

#include "mertarget.h"

#include "merconstants.h"
#include "mericons.h"
#include "merlogging.h"
#include "merqtversion.h"
#include "mersdk.h"
#include "mersdkkitinformation.h"
#include "mersdkmanager.h"
#include "mertargetkitinformation.h"
#include "mertoolchain.h"

#include <coreplugin/icore.h>
#include <debugger/debuggeritem.h>
#include <debugger/debuggeritemmanager.h>
#include <debugger/debuggerkitinformation.h>
#include <projectexplorer/toolchainmanager.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtversionfactory.h>
#include <qtsupport/qtversionmanager.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QStringList>

using namespace Debugger;
using namespace ProjectExplorer;
using namespace QtSupport;
using namespace Utils;

namespace Mer {
namespace Internal {

using namespace Mer::Constants;

const char* wrapperScripts[] =
{
    MER_WRAPPER_QMAKE,
    MER_WRAPPER_MAKE,
    MER_WRAPPER_GCC,
    MER_WRAPPER_DEPLOY,
    MER_WRAPPER_RPM,
    MER_WRAPPER_RPMBUILD,
    MER_WRAPPER_RPMVALIDATION,
    MER_WRAPPER_LUPDATE,
};

bool MerRpmValidationSuiteData::operator==(const MerRpmValidationSuiteData &other) const
{
    return id == other.id && name == other.name && website == other.website && essential == other.essential;
}

MerTarget::MerTarget(MerSdk* mersdk):
    m_sdk(mersdk)
{
}

MerTarget::~MerTarget()
{
}

QString MerTarget::name() const
{
    return m_name;
}

void MerTarget::setName(const QString &name)
{
    m_name = name;
}

void MerTarget::setQmakeQuery(const QString &qmakeQuery)
{
    m_qmakeQuery = qmakeQuery;
}

void MerTarget::setGccDumpMachine(const QString &gccMachineDump)
{
    m_gccMachineDump = gccMachineDump;
    // the dump contains a linefeed
    m_gccMachineDump.remove(QRegExp(QLatin1String("\\n")));
}

void MerTarget::setRpmValidationSuites(const QString &rpmValidationSuites)
{
    m_rpmValidationSuites = rpmValidationSuitesFromString(rpmValidationSuites, &m_rpmValidationSuitesIsValid);
}

void MerTarget::setDefaultGdb(const QString &name)
{
    m_defaultGdb=name;
}

QList<MerRpmValidationSuiteData> MerTarget::rpmValidationSuites() const
{
    return m_rpmValidationSuites;
}

bool MerTarget::fromMap(const QVariantMap &data)
{
    m_name = data.value(QLatin1String(Constants::MER_TARGET_NAME)).toString();
    m_qmakeQuery = data.value(QLatin1String(Constants::MER_TARGET_QMAKE_DUMP)).toString();
    m_gccMachineDump = data.value(QLatin1String(Constants::MER_TARGET_GCC_DUMP)).toString();
    QString rpmValidationSuites = data.value(QLatin1String(Constants::MER_TARGET_RPMVALIDATION_DUMP)).toString();
    m_rpmValidationSuites = rpmValidationSuitesFromString(rpmValidationSuites, &m_rpmValidationSuitesIsValid);
    m_defaultGdb = data.value(QLatin1String(Constants::MER_TARGET_DEFAULT_DEBUGGER)).toString();
    return isValid();
}

QVariantMap MerTarget::toMap() const
{
    QVariantMap result;
    result.insert(QLatin1String(Constants::MER_TARGET_NAME), m_name);
    result.insert(QLatin1String(Constants::MER_TARGET_QMAKE_DUMP), m_qmakeQuery);
    result.insert(QLatin1String(Constants::MER_TARGET_GCC_DUMP), m_gccMachineDump);
    result.insert(QLatin1String(Constants::MER_TARGET_RPMVALIDATION_DUMP),
            rpmValidationSuitesToString(m_rpmValidationSuites));
    result.insert(QLatin1String(Constants::MER_TARGET_DEFAULT_DEBUGGER), m_defaultGdb);
    return result;
}

bool MerTarget::isValid() const
{
    return m_sdk && !m_name.isEmpty() && !m_qmakeQuery.isEmpty() && !m_gccMachineDump.isEmpty()
        && m_rpmValidationSuitesIsValid;
}

QString MerTarget::targetPath() const
{
    return MerSdkManager::sdkToolsDirectory() + m_sdk->virtualMachineName() + QLatin1Char('/') + m_name;
}

bool MerTarget::createScripts() const
{
    if (!isValid())
        return false;

    const QString targetPath(this->targetPath());

    QDir targetDir(targetPath);
    if (!targetDir.exists() && !targetDir.mkpath(targetPath)) {
        qWarning() << "Could not create target directory." << targetDir;
        return false;
    }
    bool result = true;
    for (size_t i = 0; i < sizeof(wrapperScripts) / sizeof(wrapperScripts[0]); ++i)
         result &= createScript(targetPath, i);

    const QString qmakepath = targetPath + QLatin1Char('/') + QLatin1String(Constants::QMAKE_QUERY);
    const QString gccpath = targetPath + QLatin1Char('/') + QLatin1String(Constants::GCC_DUMPMACHINE);

    QString patchedQmakeQuery = m_qmakeQuery;
    patchedQmakeQuery.replace(QLatin1String(":/"),
                              QString::fromLatin1(":%1/%2/").arg(m_sdk->sharedTargetsPath()).arg(m_name));

    result &= createCacheFile(qmakepath, patchedQmakeQuery);
    result &= createCacheFile(gccpath, m_gccMachineDump);

    return result;
}

void MerTarget::deleteScripts() const
{
    FileUtils::removeRecursively(FileName::fromString(targetPath()));
}

Kit* MerTarget::createKit() const
{
    if (!isValid())
        return 0;
    const QString sysroot(m_sdk->sharedTargetsPath() + QLatin1Char('/') + m_name);

    FileName path = FileName::fromString(sysroot);
    if (!path.toFileInfo().exists()) {
        qWarning() << "Sysroot does not exist" << sysroot;
        return 0;
    }

    Kit *k = new Kit();
    k->setAutoDetected(true);
    k->setUnexpandedDisplayName(QString::fromLatin1("%1 (in %2)").arg(m_name, m_sdk->virtualMachineName()));
    SysRootKitInformation::setSysRoot(k, FileName::fromUserInput(sysroot));

    DeviceTypeKitInformation::setDeviceTypeId(k, Constants::MER_DEVICE_TYPE);
    k->setMutable(DeviceKitInformation::id(), true);

    ensureDebuggerIsSet(k);

    MerSdkKitInformation::setSdk(k,m_sdk);
    MerTargetKitInformation::setTargetName(k,name());
    return k;
}

void MerTarget::ensureDebuggerIsSet(Kit *k) const
{
    QTC_ASSERT(!m_defaultGdb.isEmpty(), return);

    const QString gdb = HostOsInfo::withExecutableSuffix(m_defaultGdb);
    QString gdbDir = QCoreApplication::applicationDirPath();
    if (HostOsInfo::isMacHost()) {
        QDir dir = QDir(gdbDir);
        dir.cdUp();
        dir.cdUp();
        dir.cdUp();
        gdbDir = dir.path();
    }
    FileName gdbFileName = FileName::fromString(gdbDir + QLatin1Char('/') + gdb);

    if (gdbFileName.toFileInfo().exists()) {
        if (const DebuggerItem *existing = DebuggerItemManager::findByCommand(gdbFileName)) {
            DebuggerKitInformation::setDebugger(k, existing->id());
        } else {
            DebuggerItem debugger;
            debugger.setCommand(gdbFileName);
            debugger.setEngineType(GdbEngineType);
            debugger.setUnexpandedDisplayName(QObject::tr("GDB (%1)").arg(m_defaultGdb));
            debugger.setAutoDetected(true);
            const int prefixLength = QString(Constants::MER_DEBUGGER_FILENAME_PREFIX).length();
            debugger.setAbi(Abi::abiFromTargetTriplet(m_defaultGdb.mid(prefixLength)));
            QVariant id = DebuggerItemManager::registerDebugger(debugger);
            DebuggerKitInformation::setDebugger(k, id);
        }
    } else {
        qCWarning(Log::sdks) << "Debugger binary" << gdb << "not found";
        k->setValue(DebuggerKitInformation::id(), QVariant(QString()));
    }
}

MerQtVersion* MerTarget::createQtVersion() const
{
    const FileName qmake = FileName::fromString(targetPath() + QLatin1Char('/') +
            QLatin1String(Constants::MER_WRAPPER_QMAKE));
    // Is there a qtversion present for this qmake?
    BaseQtVersion *qtv = QtVersionManager::qtVersionForQMakeBinary(qmake);
    if (qtv && !qtv->isValid()) {
        QtVersionManager::removeVersion(qtv);
        qtv = 0;
    }
    if (!qtv)
        qtv = new MerQtVersion(qmake, true, targetPath());

    //QtVersionFactory::createQtVersionFromQMakePath(qmake, true, targetPath());

    QTC_ASSERT(qtv && qtv->type() == QLatin1String(Constants::MER_QT), return 0);

    MerQtVersion *merqtv = static_cast<MerQtVersion *>(qtv);
    const QString vmName = m_sdk->virtualMachineName();
    merqtv->setVirtualMachineName(vmName);
    merqtv->setTargetName(m_name);
    merqtv->setUnexpandedDisplayName(
                QString::fromLatin1("Qt %1 for %2 in %3").arg(qtv->qtVersionString(),
                                                              m_name, vmName));
    return merqtv;
}

MerToolChain* MerTarget::createToolChain(Core::Id l) const
{
    const FileName gcc = FileName::fromString(targetPath() + QLatin1Char('/') +
            QLatin1String(Constants::MER_WRAPPER_GCC));
    QList<ToolChain *> toolChains = ToolChainManager::toolChains();

    foreach (ToolChain *tc, toolChains) {
        if (tc->compilerCommand() == gcc && tc->isAutoDetected()) {
            QTC_ASSERT(tc->typeId() == Constants::MER_TOOLCHAIN_ID, return 0);
        }
    }

    MerToolChain* mertoolchain = new MerToolChain(ToolChain::AutoDetection);
    const QString vmName = m_sdk->virtualMachineName();
    mertoolchain->setDisplayName(QString::fromLatin1("GCC (%1 in %2)").arg(m_name, vmName));
    mertoolchain->setVirtualMachine(vmName);
    mertoolchain->setTargetName(m_name);
    mertoolchain->setLanguage(l);
    mertoolchain->resetToolChain(gcc);
    return mertoolchain;
}

bool MerTarget::createScript(const QString &targetPath, int scriptIndex) const
{
    bool ok = false;
    const char* wrapperScriptCopy = wrapperScripts[scriptIndex];
    const QFile::Permissions rwrw = QFile::ReadOwner|QFile::ReadUser|QFile::ReadGroup
            |QFile::WriteOwner|QFile::WriteUser|QFile::WriteGroup;
    const QFile::Permissions rwxrwx = rwrw|QFile::ExeOwner|QFile::ExeUser|QFile::ExeGroup;

    QString wrapperScriptCommand = QLatin1String(wrapperScriptCopy);
    if (HostOsInfo::isWindowsHost())
        wrapperScriptCommand.chop(4); // remove the ".cmd"

    const QString wrapperBinaryPath = Core::ICore::libexecPath() + QLatin1String("/merssh") + QStringLiteral(QTC_HOST_EXE_SUFFIX);

    using namespace Utils;
    const QString scriptCopyPath = targetPath + QLatin1Char('/') + QLatin1String(wrapperScriptCopy);
    QDir targetDir(targetPath);
    const QString targetName = targetDir.dirName();
    targetDir.cdUp();
    const QString merDevToolsDir = QDir::toNativeSeparators(targetDir.canonicalPath());
    QFile script(scriptCopyPath);
    ok = script.open(QIODevice::WriteOnly);
    if (!ok) {
        qWarning() << "Could not open script" << scriptCopyPath;
        return false;
    }

    QString scriptContent;

    if (HostOsInfo::isWindowsHost()) {
        scriptContent += QLatin1String("@echo off\nSetLocal EnableDelayedExpansion\n");
        scriptContent += QLatin1String("set ARGUMENTS=\nFOR %%a IN (%*) DO set ARGUMENTS=!ARGUMENTS! ^ '%%a'\n");
        scriptContent += QLatin1String("set ") +
                QLatin1String(MER_SSH_TARGET_NAME) +
                QLatin1Char('=') + targetName + QLatin1Char('\n');
        scriptContent += QLatin1String("set ") +
                QLatin1String(MER_SSH_SDK_TOOLS) +
                QLatin1Char('=') + merDevToolsDir + QDir::separator() + targetName + QLatin1Char('\n');
        scriptContent += QLatin1String("SetLocal DisableDelayedExpansion\n");
        scriptContent += QLatin1Char('"') +
                QDir::toNativeSeparators(wrapperBinaryPath) + QLatin1String("\" ") +
                wrapperScriptCommand + QLatin1Char(' ') + QLatin1String("%ARGUMENTS%\n");
    }

    if (HostOsInfo::isAnyUnixHost()) {
        scriptContent += QLatin1String("#!/bin/sh\n");
        scriptContent += QLatin1String("ARGUMENTS=\"\";for ARGUMENT in \"$@\"; do ARGUMENTS=\"${ARGUMENTS} '${ARGUMENT}'\" ; done;\n");
        scriptContent += QLatin1String("export  ") +
                QLatin1String(MER_SSH_TARGET_NAME) +
                QLatin1Char('=') + targetName + QLatin1Char('\n');
        scriptContent += QLatin1String("export  ") +
                QLatin1String(MER_SSH_SDK_TOOLS) +
                QLatin1String("=\"") + merDevToolsDir + QDir::separator() + targetName + QLatin1String("\"\n");
        scriptContent += QLatin1String("exec ");
        scriptContent += QLatin1Char('"') +
                QDir::toNativeSeparators(wrapperBinaryPath) + QLatin1String("\" ") +
                wrapperScriptCommand + QLatin1Char(' ') + QLatin1String("${ARGUMENTS}");
    }


    ok = script.write(scriptContent.toUtf8()) != -1;
    if (!ok) {
        qWarning() << "Could not write script" << scriptCopyPath;
        return false;
    }

    ok = QFile::setPermissions(scriptCopyPath, rwxrwx);
    if (!ok) {
        qWarning() << "Could not set file permissions on script" << scriptCopyPath;
        return false;
    }
    return ok;
}

bool MerTarget::createCacheFile(const QString &fileName, const QString &content) const
{
    bool ok = false;

    QFile file(fileName);
    ok = file.open(QIODevice::WriteOnly);
    if (!ok) {
        qWarning() << "Could not open cache file" << fileName;
        return false;
    }

    ok = file.write(content.toUtf8()) != -1;
    if (!ok) {
        qWarning() << "Could not write cache file " << fileName;
        return false;
    }
    file.close();
    return ok;
}

QList<MerRpmValidationSuiteData> MerTarget::rpmValidationSuitesFromString(const QString &string, bool *ok)
{
    QList<MerRpmValidationSuiteData> retv;
    QString string_(string);
    QTextStream in(&string_);
    QString line;
    *ok = true;
    while (in.readLineInto(&line)) {
        MerRpmValidationSuiteData data;

        QStringList split = line.split(' ', QString::SkipEmptyParts);
        if (split.count() < 3) {
            qWarning() << "Error parsing listing of RPM validation suites: The corrupted line is:" << line;
            *ok = false;
            break;
        }

        data.id = split.takeFirst();
        data.essential = split.takeFirst().toLower() == "essential";
        data.website = split.takeFirst();
        if (data.website == "-")
            data.website.clear();
        data.name = split.join(' ');

        retv.append(data);
    }

    return retv;
}

QString MerTarget::rpmValidationSuitesToString(const QList<MerRpmValidationSuiteData> &suites)
{
    QString retv;
    QTextStream out(&retv);
    for (const auto &suite : suites) {
        out << suite.id << ' '
            << (suite.essential ? "Essential " : "Optional ")
            << (suite.website.isEmpty() ? QString("-") : suite.website) << ' '
            << suite.name
            << endl;
    }
    return retv;
}

bool MerTarget::operator==(const MerTarget &other) const
{
    return m_name == other.m_name && m_qmakeQuery == other.m_qmakeQuery && m_gccMachineDump == other.m_gccMachineDump
        && m_rpmValidationSuites == other.m_rpmValidationSuites;
}

}
}
