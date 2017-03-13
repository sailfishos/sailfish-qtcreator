/****************************************************************************
**
** Copyright (C) 2016 BlackBerry Limited. All rights reserved.
** Contact: BlackBerry (qt@blackberry.com), KDAB (info@kdab.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "qnxtoolchain.h"
#include "qnxconstants.h"
#include "qnxutils.h"

#include <utils/pathchooser.h>

#include <QFormLayout>

using namespace ProjectExplorer;
using namespace Utils;

namespace Qnx {
namespace Internal {

static const char CompilernNdkPath[] = "Qnx.QnxToolChain.NDKPath";

static const QList<Abi> qccSupportedAbis()
{
    QList<Abi> abis;
    abis << Abi(Abi::ArmArchitecture, Abi::LinuxOS, Abi::GenericLinuxFlavor, Abi::ElfFormat, 32);
    abis << Abi(Abi::X86Architecture, Abi::LinuxOS, Abi::GenericLinuxFlavor, Abi::ElfFormat, 32);

    return abis;
}

static void setQnxEnvironment(Environment &env, const QList<EnvironmentItem> &qnxEnv)
{
    // We only need to set QNX_HOST and QNX_TARGET needed when running qcc
    foreach (const EnvironmentItem &item, qnxEnv) {
        if (item.name == QLatin1String("QNX_HOST") ||
                item.name == QLatin1String("QNX_TARGET") )
            env.set(item.name, item.value);
    }
}

QnxToolChain::QnxToolChain(ToolChain::Detection d)
    : GccToolChain(Constants::QNX_TOOLCHAIN_ID, d)
{ }

QString QnxToolChain::typeDisplayName() const
{
    return QnxToolChainFactory::tr("QCC");
}

ToolChainConfigWidget *QnxToolChain::configurationWidget()
{
    return new QnxToolChainConfigWidget(this);
}

void QnxToolChain::addToEnvironment(Environment &env) const
{
    if (env.value(QLatin1String("QNX_HOST")).isEmpty()
            || env.value(QLatin1String("QNX_TARGET")).isEmpty())
        setQnxEnvironment(env, QnxUtils::qnxEnvironment(m_ndkPath));

    GccToolChain::addToEnvironment(env);
}

FileNameList QnxToolChain::suggestedMkspecList() const
{
    FileNameList mkspecList;
    mkspecList << FileName::fromLatin1("qnx-armv7le-qcc");
    mkspecList << FileName::fromLatin1("qnx-armle-v7-qcc");
    mkspecList << FileName::fromLatin1("qnx-x86-qcc");

    return mkspecList;
}

QVariantMap QnxToolChain::toMap() const
{
    QVariantMap data = GccToolChain::toMap();
    data.insert(QLatin1String(CompilernNdkPath), m_ndkPath);
    return data;
}

bool QnxToolChain::fromMap(const QVariantMap &data)
{
    if (!GccToolChain::fromMap(data))
        return false;

    m_ndkPath = data.value(QLatin1String(CompilernNdkPath)).toString();
    return true;
}

QString QnxToolChain::ndkPath() const
{
    return m_ndkPath;
}

void QnxToolChain::setNdkPath(const QString &ndkPath)
{
    if (m_ndkPath == ndkPath)
        return;
    m_ndkPath = ndkPath;
    toolChainUpdated();
}

// qcc doesn't support a "-dumpmachine" option to get supported abis
GccToolChain::DetectedAbisResult QnxToolChain::detectSupportedAbis() const
{
    return qccSupportedAbis();
}

// Qcc is a multi-compiler driver, and most of the gcc options can be accomplished by using the -Wp, and -Wc
// options to pass the options directly down to the compiler
QStringList QnxToolChain::reinterpretOptions(const QStringList &args) const
{
    QStringList arguments;
    foreach (const QString &str, args) {
        if (str.startsWith(QLatin1String("--sysroot=")))
            continue;
        QString arg = str;
        if (arg == QLatin1String("-v")
            || arg == QLatin1String("-dM"))
                arg.prepend(QLatin1String("-Wp,"));
        arguments << arg;
    }

    return arguments;
}

// --------------------------------------------------------------------------
// QnxToolChainFactory
// --------------------------------------------------------------------------

QnxToolChainFactory::QnxToolChainFactory()
{
    setDisplayName(tr("QCC"));
}

bool QnxToolChainFactory::canRestore(const QVariantMap &data)
{
    return typeIdFromMap(data) == Constants::QNX_TOOLCHAIN_ID;
}

ToolChain *QnxToolChainFactory::restore(const QVariantMap &data)
{
    QnxToolChain *tc = new QnxToolChain(ToolChain::ManualDetection);
    if (tc->fromMap(data))
        return tc;

    delete tc;
    return 0;
}

bool QnxToolChainFactory::canCreate()
{
    return true;
}

ToolChain *QnxToolChainFactory::create()
{
    return new QnxToolChain(ToolChain::ManualDetection);
}

//---------------------------------------------------------------------------------
// QnxToolChainConfigWidget
//---------------------------------------------------------------------------------

QnxToolChainConfigWidget::QnxToolChainConfigWidget(QnxToolChain *tc)
    : ToolChainConfigWidget(tc)
    , m_compilerCommand(new PathChooser)
    , m_ndkPath(new PathChooser)
    , m_abiWidget(new AbiWidget)
{
    m_compilerCommand->setExpectedKind(PathChooser::ExistingCommand);
    m_compilerCommand->setHistoryCompleter(QLatin1String("Qnx.ToolChain.History"));
    m_compilerCommand->setFileName(tc->compilerCommand());
    m_compilerCommand->setEnabled(!tc->isAutoDetected());

    m_ndkPath->setExpectedKind(PathChooser::ExistingDirectory);
    m_ndkPath->setHistoryCompleter(QLatin1String("Qnx.Ndk.History"));
    m_ndkPath->setPath(tc->ndkPath());
    m_ndkPath->setEnabled(!tc->isAutoDetected());

    m_abiWidget->setAbis(qccSupportedAbis(), tc->targetAbi());
    m_abiWidget->setEnabled(!tc->isAutoDetected());

    m_mainLayout->addRow(tr("&Compiler path:"), m_compilerCommand);
    //: SDP refers to 'Software Development Platform'.
    m_mainLayout->addRow(tr("NDK/SDP path:"), m_ndkPath);
    m_mainLayout->addRow(tr("&ABI:"), m_abiWidget);

    connect(m_compilerCommand, &PathChooser::rawPathChanged, this, &ToolChainConfigWidget::dirty);
    connect(m_ndkPath, &PathChooser::rawPathChanged, this, &ToolChainConfigWidget::dirty);
    connect(m_abiWidget, &AbiWidget::abiChanged, this, &ToolChainConfigWidget::dirty);
}

void QnxToolChainConfigWidget::applyImpl()
{
    if (toolChain()->isAutoDetected())
        return;

    QnxToolChain *tc = static_cast<QnxToolChain *>(toolChain());
    Q_ASSERT(tc);
    QString displayName = tc->displayName();
    tc->resetToolChain(m_compilerCommand->fileName());
    tc->setDisplayName(displayName); // reset display name
    tc->setNdkPath(m_ndkPath->fileName().toString());
    tc->setTargetAbi(m_abiWidget->currentAbi());
}

void QnxToolChainConfigWidget::discardImpl()
{
    // subwidgets are not yet connected!
    bool blocked = blockSignals(true);
    QnxToolChain *tc = static_cast<QnxToolChain *>(toolChain());
    m_compilerCommand->setFileName(tc->compilerCommand());
    m_ndkPath->setPath(tc->ndkPath());
    m_abiWidget->setAbis(tc->supportedAbis(), tc->targetAbi());
    if (!m_compilerCommand->path().isEmpty())
        m_abiWidget->setEnabled(true);
    blockSignals(blocked);
}

bool QnxToolChainConfigWidget::isDirtyImpl() const
{
    QnxToolChain *tc = static_cast<QnxToolChain *>(toolChain());
    Q_ASSERT(tc);
    return m_compilerCommand->fileName() != tc->compilerCommand()
            || m_ndkPath->path() != tc->ndkPath()
            || m_abiWidget->currentAbi() != tc->targetAbi();
}

} // namespace Internal
} // namespace Qnx
