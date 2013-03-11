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

#include "sdktoolchainutils.h"
#include "mersdkmanager.h"
#include "merconstants.h"
#include "mertoolchain.h"

#include <projectexplorer/toolchainmanager.h>
#include <projectexplorer/toolchain.h>
#include <ssh/sshremoteprocessrunner.h>
#include <utils/qtcassert.h>

#include <QFileInfo>

namespace Mer {
namespace Internal {

SdkToolChainUtils::SdkToolChainUtils(QObject *parent)
    : QObject(parent)
    , m_remoteProcessRunner(new QSsh::SshRemoteProcessRunner(this))
{
    connect(m_remoteProcessRunner, SIGNAL(connectionError()), SLOT(onConnectionError()));
    connect(m_remoteProcessRunner, SIGNAL(readyReadStandardOutput()),
            SLOT(onReadReadyStandardOutput()));
    connect(m_remoteProcessRunner, SIGNAL(readyReadStandardError()),
            SLOT(onReadReadyStandardError()));
    connect(m_remoteProcessRunner, SIGNAL(processClosed(int)), SLOT(onProcessClosed(int)));
}

void SdkToolChainUtils::setSshConnectionParameters(const QSsh::SshConnectionParameters params)
{
    m_sshParameters = params;
}

bool SdkToolChainUtils::checkInstalledToolChains(const QString &sdkName)
{
    if (m_remoteProcessRunner->isProcessRunning())
        return false;
    m_output.clear();
    m_sdkName = sdkName;
    m_remoteProcessRunner->run("sdk-manage --toolchain --list", m_sshParameters);
    return true;
}

void SdkToolChainUtils::updateToolChains(MerSdk &sdk, const QStringList &installedTargets)
{
    const QString sdkToolsDir(MerSdkManager::instance()->sdkToolsDirectory()
                              + sdk.virtualMachineName() + QLatin1Char('/'));
    ProjectExplorer::ToolChainManager *tcm = ProjectExplorer::ToolChainManager::instance();

    QHash<QString, QString> updatedToolChains;
    QStringList targets = installedTargets;

    // remove old versions
    QHashIterator<QString, QString> i(sdk.toolChains());
    while (i.hasNext()) {
        i.next();
        QString t = i.key();
        if (targets.contains(t)) {
            targets.removeOne(t);
            updatedToolChains.insert(i.key(), i.value());
            continue;
        }
        ProjectExplorer::ToolChain *toolChain = tcm->findToolChain(i.value());
        if (toolChain)
            tcm->deregisterToolChain(toolChain);
    }

    // create new tool chains
    foreach (const QString &t, targets) {
        MerToolChain *tc = createToolChain(sdkToolsDir + t);
        if (tc) {
            const QString vmName = sdk.virtualMachineName();
            tc->setDisplayName(QString::fromLocal8Bit("GCC (%1 %2)").arg(vmName, t));
            tc->setVirtualMachine(vmName);
            tc->setTargetName(t);
            tcm->registerToolChain(tc);
            updatedToolChains.insert(t, tc->id());
        }
    }

    sdk.setToolChains(updatedToolChains);
}

void SdkToolChainUtils::onReadReadyStandardOutput()
{
    const QString output = QString::fromLocal8Bit(m_remoteProcessRunner->readAllStandardOutput());
    m_output.append(output);
    emit processOutput(output);
}

void SdkToolChainUtils::onReadReadyStandardError()
{
    const QString output = QString::fromLocal8Bit(m_remoteProcessRunner->readAllStandardError());
    emit processOutput(output);
}

void SdkToolChainUtils::onProcessClosed(int exitCode)
{
    QRegExp rx(QLatin1String("\\b(.*-.*-.*),(.)\\b"));
    rx.setMinimal(true);
    int pos = 0;
    QStringList toolChains;
    while ((pos = rx.indexIn(m_output, pos)) != -1) {
        if (rx.cap(2) == QLatin1String("i"))
            toolChains.append(rx.cap(1));
        pos += rx.matchedLength();
    }
    emit installedToolChains(m_sdkName, toolChains);
    QTC_ASSERT(exitCode == QSsh::SshRemoteProcess::NormalExit, return);
}

MerToolChain *SdkToolChainUtils::createToolChain(const QString &targetToolsDir)
{
    const Utils::FileName gcc =
            Utils::FileName::fromString(targetToolsDir + QLatin1Char('/') +
                                        QLatin1String(Constants::MER_WRAPPER_GCC));
    ProjectExplorer::ToolChainManager *tcm = ProjectExplorer::ToolChainManager::instance();
    QList<ProjectExplorer::ToolChain *> toolChains = tcm->toolChains();

    foreach (ProjectExplorer::ToolChain *tc, toolChains) {
        if (tc->compilerCommand() == gcc && tc->isAutoDetected()) {
            QTC_ASSERT(tc->type() == QLatin1String(Constants::MER_TOOLCHAIN_TYPE), return 0);
            return static_cast<MerToolChain *>(tc);
        }
    }

    return new MerToolChain(true, gcc);
}

void SdkToolChainUtils::onConnectionError()
{
    emit connectionError();
}

} // Internal
} // Mer
