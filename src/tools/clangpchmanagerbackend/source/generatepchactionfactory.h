/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include <clang/Tooling/Tooling.h>

#include <clang/Basic/DiagnosticOptions.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Lex/PreprocessorOptions.h>

namespace ClangBackEnd {

class GeneratePCHAction final : public clang::GeneratePCHAction
{
public:
    GeneratePCHAction(llvm::StringRef filePath, llvm::StringRef fileContent)
        : m_filePath(filePath)
        , m_fileContent(fileContent)
    {}

    bool BeginInvocation(clang::CompilerInstance &compilerInstance) override
    {
        compilerInstance.getPreprocessorOpts().DisablePCHValidation = true;
        compilerInstance.getPreprocessorOpts().AllowPCHWithCompilerErrors = true;
        compilerInstance.getDiagnosticOpts().ErrorLimit = 0;
        compilerInstance.getFrontendOpts().SkipFunctionBodies = true;
        compilerInstance.getFrontendOpts().IncludeTimestamps = true;
        std::unique_ptr<llvm::MemoryBuffer> Input = llvm::MemoryBuffer::getMemBuffer(m_fileContent);
        compilerInstance.getPreprocessorOpts().addRemappedFile(m_filePath, Input.release());

        return clang::GeneratePCHAction::BeginSourceFileAction(compilerInstance);
    }

private:
    llvm::StringRef m_filePath;
    llvm::StringRef m_fileContent;
};

class GeneratePCHActionFactory final : public clang::tooling::FrontendActionFactory
{
public:
    GeneratePCHActionFactory(llvm::StringRef filePath, llvm::StringRef fileContent)
        : m_filePath(filePath)
        , m_fileContent(fileContent)
    {}

#if LLVM_VERSION_MAJOR >= 10
    std::unique_ptr<clang::FrontendAction> create() override
    {
        return std::make_unique<GeneratePCHAction>(m_filePath, m_fileContent);
    }
#else
    clang::FrontendAction *create() override
    {
        return new GeneratePCHAction{m_filePath, m_fileContent};
    }
#endif

private:
    llvm::StringRef m_filePath;
    llvm::StringRef m_fileContent;
};
} // namespace ClangBackEnd
