/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "clangbackendcommunicator.h"
#include "clangutils.h"

#include <texteditor/codeassist/assistinterface.h>

namespace ClangCodeModel {
namespace Internal {

enum class CompletionType { FunctionHint, Other };

class ClangCompletionAssistInterface: public TextEditor::AssistInterface
{
public:
    ClangCompletionAssistInterface(BackendCommunicator &communicator,
                                   CompletionType type,
                                   const TextEditor::TextEditorWidget *textEditorWidget,
                                   int position,
                                   const Utils::FilePath &fileName,
                                   TextEditor::AssistReason reason,
                                   const ProjectExplorer::HeaderPaths &headerPaths,
                                   const CPlusPlus::LanguageFeatures &features);

    BackendCommunicator &communicator() const;
    CompletionType type() const { return m_type; }
    bool objcEnabled() const;
    const ProjectExplorer::HeaderPaths &headerPaths() const;
    CPlusPlus::LanguageFeatures languageFeatures() const;
    const TextEditor::TextEditorWidget *textEditorWidget() const;

    void setHeaderPaths(const ProjectExplorer::HeaderPaths &headerPaths); // For tests

private:
    BackendCommunicator &m_communicator;
    const CompletionType m_type;
    QStringList m_options;
    ProjectExplorer::HeaderPaths m_headerPaths;
    CPlusPlus::LanguageFeatures m_languageFeatures;
    const TextEditor::TextEditorWidget *m_textEditorWidget;
};

} // namespace Internal
} // namespace ClangCodeModel
