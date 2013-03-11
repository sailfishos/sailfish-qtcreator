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

#include "sdkscriptsutils.h"
#include "mersdkmanager.h"
#include "merconstants.h"

#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <utils/hostosinfo.h>

#include <QDir>

namespace Mer {
namespace Internal {

static void createScripts(const QString &targetPath);
static bool createScript(const QString &targetPath, int scriptIndex);

static const struct WrapperScript {
    enum Execution {
        ExecutionStandard,
        ExecutionTypeSb2
    };
    const char* name;
    Execution executionType;
} wrapperScripts[] = {
    { Constants::MER_WRAPPER_QMAKE, WrapperScript::ExecutionTypeSb2 },
    { Constants::MER_WRAPPER_MAKE, WrapperScript::ExecutionTypeSb2 },
    { Constants::MER_WRAPPER_GCC, WrapperScript::ExecutionTypeSb2 },
    { Constants::MER_WRAPPER_GDB, WrapperScript::ExecutionTypeSb2 },
    { Constants::MER_WRAPPER_SPECIFY, WrapperScript::ExecutionStandard },
    { Constants::MER_WRAPPER_MB, WrapperScript::ExecutionStandard },
    { Constants::MER_WRAPPER_RM, WrapperScript::ExecutionStandard },
    { Constants::MER_WRAPPER_MV, WrapperScript::ExecutionStandard }
};

void SdkScriptsUtils::removeSdkScripts(const MerSdk &sdk)
{
    const QString sdkToolsDir(MerSdkManager::instance()->sdkToolsDirectory()
                              + sdk.virtualMachineName() + QLatin1Char('/'));
    Utils::FileUtils::removeRecursively(Utils::FileName::fromString(sdkToolsDir));
}

void SdkScriptsUtils::removeInvalidScripts(const MerSdk &sdk, const QStringList &targets)
{
    const QString sdkToolsDir(MerSdkManager::instance()->sdkToolsDirectory()
                              + sdk.virtualMachineName() + QLatin1Char('/'));

    QStringList targetList = targets;
    // remove old scripts
    QHashIterator<QString, int> i(sdk.qtVersions());
    while (i.hasNext()) {
        i.next();
        QString t = i.key();
        if (targetList.contains(t)) {
            targetList.removeOne(t);
            continue;
        }
        Utils::FileUtils::removeRecursively(Utils::FileName::fromString(sdkToolsDir + t));
    }
}

void SdkScriptsUtils::createNewScripts(const MerSdk &sdk, const QStringList &targets)
{
    const QString sdkToolsDir(MerSdkManager::instance()->sdkToolsDirectory()
                              + sdk.virtualMachineName() + QLatin1Char('/'));

    QStringList presentTargets = sdk.qtVersions().keys();
    // create new scripts
    foreach (const QString &t, targets) {
        if (presentTargets.contains(t))
            continue;
        createScripts(sdkToolsDir + t);
    }
}

void createScripts(const QString &targetPath)
{
    QDir targetDir(targetPath);
    if (!targetDir.exists() && !targetDir.mkpath(targetPath))
        return;
    for (size_t i = 0; i < sizeof(wrapperScripts) / sizeof(wrapperScripts[0]); ++i)
        QTC_ASSERT(createScript(targetPath, i), continue);
}

bool createScript(const QString &targetPath, int scriptIndex)
{
    bool ok = false;
    const WrapperScript &wrapperScriptCopy = wrapperScripts[scriptIndex];
    const QFile::Permissions rwrw = QFile::ReadOwner|QFile::ReadUser|QFile::ReadGroup
            |QFile::WriteOwner|QFile::WriteUser|QFile::WriteGroup;
    const QFile::Permissions rwxrwx = rwrw|QFile::ExeOwner|QFile::ExeUser|QFile::ExeGroup;

    QString wrapperScriptCommand = QLatin1String(wrapperScriptCopy.name);
    if (Utils::HostOsInfo::isWindowsHost())
        wrapperScriptCommand.chop(4); // remove the ".cmd"

    QString wrapperBinaryPath = QCoreApplication::applicationDirPath();
    if (Utils::HostOsInfo::isWindowsHost())
        wrapperBinaryPath += QLatin1String("/merssh.exe");
    else if (Utils::HostOsInfo::isMacHost())
        wrapperBinaryPath += QLatin1String("/../Resources/merssh");
    else
        wrapperBinaryPath += QLatin1String("/merssh");

    using namespace Utils;
    const QString allParameters = HostOsInfo::isWindowsHost() ? QLatin1String("%*")
                                                              : QLatin1String("$@");

    const QString scriptCopyPath = targetPath + QLatin1Char('/')
            + QLatin1String(wrapperScriptCopy.name);
    QDir targetDir(targetPath);
    const QString targetName = targetDir.dirName();
    targetDir.cdUp();
    const QString merDevToolsDir = targetDir.canonicalPath();
    QFile script(scriptCopyPath);
    ok = script.open(QIODevice::WriteOnly);
    if (!ok)
        return false;
    const QString executionType =
            QLatin1String(wrapperScriptCopy.executionType == WrapperScript::ExecutionStandard ?
                              Constants::MER_EXECUTIONTYPE_STANDARD
                            : Constants::MER_EXECUTIONTYPE_SB2);
    const QString scriptHeader = HostOsInfo::isWindowsHost() ? QLatin1String("@echo off\n")
                                                             : QLatin1String("#!/bin/bash\n");
    const QString scriptContent = scriptHeader
            + QLatin1Char('"') + QDir::toNativeSeparators(wrapperBinaryPath) + QLatin1String("\" ")
            + QLatin1String(Mer::Constants::MERSSH_PARAMETER_SDKTOOLSDIR) + QLatin1String(" \"")
            + merDevToolsDir + QLatin1String("\" ")
            + QLatin1String(Constants::MERSSH_PARAMETER_COMMANDTYPE) + QLatin1Char(' ')
            + executionType + QLatin1Char(' ')
            + QLatin1String(Mer::Constants::MERSSH_PARAMETER_MERTARGET) + QLatin1Char(' ')
            + targetName + QLatin1Char(' ')
            + wrapperScriptCommand + QLatin1Char(' ')
            + allParameters;
    ok = script.write(scriptContent.toUtf8()) != -1;
    if (!ok)
        return false;

    ok = QFile::setPermissions(scriptCopyPath, rwxrwx);

    return ok;
}

} // Internal
} // Mer
