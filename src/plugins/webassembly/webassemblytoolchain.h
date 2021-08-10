/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
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

#pragma once

#include <projectexplorer/gcctoolchain.h>

#include <QVersionNumber>

namespace WebAssembly {
namespace Internal {

class WebAssemblyToolChain final : public ProjectExplorer::GccToolChain
{
    Q_DECLARE_TR_FUNCTIONS(WebAssembly::Internal::WebAssemblyToolChain)

public:
    void addToEnvironment(Utils::Environment &env) const override;

    Utils::FilePath makeCommand(const Utils::Environment &environment) const override;
    bool isValid() const override;

    static const QVersionNumber &minimumSupportedEmSdkVersion();
    static void registerToolChains();
    static bool areToolChainsRegistered();

private:
    WebAssemblyToolChain();

    friend class WebAssemblyToolChainFactory;
};

class WebAssemblyToolChainFactory : public ProjectExplorer::ToolChainFactory
{
public:
    WebAssemblyToolChainFactory();

    QList<ProjectExplorer::ToolChain *> autoDetect(
            const QList<ProjectExplorer::ToolChain *> &alreadyKnown) override;
};

} // namespace Internal
} // namespace WebAssembly
