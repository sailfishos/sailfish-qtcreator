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

#include "baseeditordocumentparser.h"
#include "cppsemanticinfo.h"
#include "cpptools_global.h"

#include <texteditor/codeassist/assistinterface.h>
#include <texteditor/texteditor.h>
#include <texteditor/textdocument.h>

#include <cplusplus/CppDocument.h>

#include <QTextEdit>

namespace TextEditor {
class TextDocument;
class QuickFixOperations;
}

namespace CppTools {

class CPPTOOLS_EXPORT BaseEditorDocumentProcessor : public QObject
{
    Q_OBJECT

public:
    BaseEditorDocumentProcessor(QTextDocument *textDocument, const QString &filePath);
    virtual ~BaseEditorDocumentProcessor();

    // Function interface to implement
    virtual void run() = 0;
    virtual void semanticRehighlight() = 0;
    virtual void recalculateSemanticInfoDetached(bool force) = 0;
    virtual CppTools::SemanticInfo recalculateSemanticInfo() = 0;
    virtual CPlusPlus::Snapshot snapshot() = 0;
    virtual BaseEditorDocumentParser::Ptr parser() = 0;
    virtual bool isParserRunning() const = 0;

    virtual TextEditor::QuickFixOperations
    extraRefactoringOperations(const TextEditor::AssistInterface &assistInterface);

    virtual bool hasDiagnosticsAt(uint line, uint column) const;
    virtual void addDiagnosticToolTipToLayout(uint line, uint column, QLayout *layout) const;

signals:
    // Signal interface to implement
    void codeWarningsUpdated(unsigned revision,
                             const QList<QTextEdit::ExtraSelection> selections,
                             const TextEditor::RefactorMarkers &refactorMarkers);

    void ifdefedOutBlocksUpdated(unsigned revision,
                                 const QList<TextEditor::BlockRange> ifdefedOutBlocks);

    void cppDocumentUpdated(const CPlusPlus::Document::Ptr document);    // TODO: Remove me
    void semanticInfoUpdated(const CppTools::SemanticInfo semanticInfo); // TODO: Remove me

protected:
    static void runParser(QFutureInterface<void> &future,
                          BaseEditorDocumentParser::Ptr parser,
                          const CppTools::WorkingCopy workingCopy);

    // Convenience
    QString filePath() const { return m_filePath; }
    unsigned revision() const { return static_cast<unsigned>(m_textDocument->revision()); }
    QTextDocument *textDocument() const { return m_textDocument; }

private:
    QString m_filePath;
    QTextDocument *m_textDocument;
};

} // namespace CppTools
