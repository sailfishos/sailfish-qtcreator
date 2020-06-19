/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "builtincursorinfo.h"

#include "cppcanonicalsymbol.h"
#include "cppcursorinfo.h"
#include "cpplocalsymbols.h"
#include "cppmodelmanager.h"
#include "cppsemanticinfo.h"
#include "cpptoolsreuse.h"

#include <cplusplus/CppDocument.h>
#include <cplusplus/Macro.h>
#include <cplusplus/TranslationUnit.h>

#include <utils/textutils.h>
#include <utils/qtcassert.h>
#include <utils/runextensions.h>

#include <QTextBlock>

using namespace CPlusPlus;
using SemanticUses = QList<CppTools::SemanticInfo::Use>;

namespace CppTools {
namespace {

CursorInfo::Range toRange(const SemanticInfo::Use &use)
{
    return {use.line, use.column, use.length};
}

CursorInfo::Range toRange(int tokenIndex, TranslationUnit *translationUnit)
{
    int line, column;
    translationUnit->getTokenPosition(tokenIndex, &line, &column);
    if (column)
        --column;  // adjust the column position.

    return {line,
            column + 1,
            translationUnit->tokenAt(tokenIndex).utf16chars()};
}

CursorInfo::Range toRange(const QTextCursor &textCursor, int utf16offset, int length)
{
    QTextCursor cursor(textCursor.document());
    cursor.setPosition(utf16offset);
    const QTextBlock textBlock = cursor.block();

    return {textBlock.blockNumber() + 1,
            cursor.position() - textBlock.position() + 1,
            length};
}

CursorInfo::Ranges toRanges(const SemanticUses &uses)
{
    CursorInfo::Ranges ranges;
    ranges.reserve(uses.size());

    for (const SemanticInfo::Use &use : uses)
        ranges.append(toRange(use));

    return ranges;
}

CursorInfo::Ranges toRanges(const QList<int> &tokenIndices, TranslationUnit *translationUnit)
{
    CursorInfo::Ranges ranges;
    ranges.reserve(tokenIndices.size());

    for (int reference : tokenIndices)
        ranges.append(toRange(reference, translationUnit));

    return ranges;
}

class FunctionDefinitionUnderCursor: protected ASTVisitor
{
    int m_line = 0;
    int m_column = 0;
    DeclarationAST *m_functionDefinition = nullptr;

public:
    explicit FunctionDefinitionUnderCursor(TranslationUnit *translationUnit)
        : ASTVisitor(translationUnit)
    { }

    DeclarationAST *operator()(AST *ast, int line, int column)
    {
        m_functionDefinition = nullptr;
        m_line = line;
        m_column = column;
        accept(ast);
        return m_functionDefinition;
    }

protected:
    bool preVisit(AST *ast) override
    {
        if (m_functionDefinition)
            return false;

        if (FunctionDefinitionAST *def = ast->asFunctionDefinition())
            return checkDeclaration(def);

        if (ObjCMethodDeclarationAST *method = ast->asObjCMethodDeclaration()) {
            if (method->function_body)
                return checkDeclaration(method);
        }

        return true;
    }

private:
    bool checkDeclaration(DeclarationAST *ast)
    {
        int startLine, startColumn;
        int endLine, endColumn;
        getTokenStartPosition(ast->firstToken(), &startLine, &startColumn);
        getTokenEndPosition(ast->lastToken() - 1, &endLine, &endColumn);

        if (m_line > startLine || (m_line == startLine && m_column >= startColumn)) {
            if (m_line < endLine || (m_line == endLine && m_column < endColumn)) {
                m_functionDefinition = ast;
                return false;
            }
        }

        return true;
    }
};

class FindUses
{
public:
    static CursorInfo find(const Document::Ptr document, const Snapshot &snapshot,
                           int line, int column, Scope *scope, const QString &expression)
    {
        FindUses findUses(document, snapshot, line, column, scope, expression);
        return findUses.doFind();
    }

private:
    FindUses(const Document::Ptr document, const Snapshot &snapshot, int line, int column,
             Scope *scope, const QString &expression)
        : m_document(document)
        , m_line(line)
        , m_column(column)
        , m_scope(scope)
        , m_expression(expression)
        , m_snapshot(snapshot)
    {
    }

    CursorInfo doFind() const
    {
        CursorInfo result;

        // findLocalUses operates with 1-based line and 0-based column
        const CppTools::SemanticInfo::LocalUseMap localUses
                = BuiltinCursorInfo::findLocalUses(m_document, m_line, m_column - 1);
        result.localUses = localUses;
        splitLocalUses(localUses, &result.useRanges, &result.unusedVariablesRanges);

        if (!result.useRanges.isEmpty()) {
            result.areUseRangesForLocalVariable = true;
            return result;
        }

        result.useRanges = findReferences();
        result.areUseRangesForLocalVariable = false;
        return result; // OK, result.unusedVariablesRanges will be passed on
    }

    void splitLocalUses(const CppTools::SemanticInfo::LocalUseMap &uses,
                        CursorInfo::Ranges *rangesForLocalVariableUnderCursor,
                        CursorInfo::Ranges *rangesForLocalUnusedVariables) const
    {
        QTC_ASSERT(rangesForLocalVariableUnderCursor, return);
        QTC_ASSERT(rangesForLocalUnusedVariables, return);

        LookupContext context(m_document, m_snapshot);

        for (auto it = uses.cbegin(), end = uses.cend(); it != end; ++it) {
            const SemanticUses &uses = it.value();

            bool good = false;
            foreach (const CppTools::SemanticInfo::Use &use, uses) {
                if (m_line == use.line && m_column >= use.column
                        && m_column <= static_cast<int>(use.column + use.length)) {
                    good = true;
                    break;
                }
            }

            if (uses.size() == 1) {
                if (!CppTools::isOwnershipRAIIType(it.key(), context))
                    rangesForLocalUnusedVariables->append(toRanges(uses)); // unused declaration
            } else if (good && rangesForLocalVariableUnderCursor->isEmpty()) {
                rangesForLocalVariableUnderCursor->append(toRanges(uses));
            }
        }
    }

    CursorInfo::Ranges findReferences() const
    {
        CursorInfo::Ranges result;
        if (!m_scope || m_expression.isEmpty())
            return result;

        TypeOfExpression typeOfExpression;
        Snapshot theSnapshot = m_snapshot;
        theSnapshot.insert(m_document);
        typeOfExpression.init(m_document, theSnapshot);
        typeOfExpression.setExpandTemplates(true);

        if (Symbol *s = CanonicalSymbol::canonicalSymbol(m_scope, m_expression, typeOfExpression)) {
            CppTools::CppModelManager *mmi = CppTools::CppModelManager::instance();
            const QList<int> tokenIndices = mmi->references(s, typeOfExpression.context());
            result = toRanges(tokenIndices, m_document->translationUnit());
        }

        return result;
    }

private:
    // Shared
    Document::Ptr m_document;

    // For local use calculation
    int m_line;
    int m_column;

    // For references calculation
    Scope *m_scope;
    QString m_expression;
    Snapshot m_snapshot;
};

bool isSemanticInfoValidExceptLocalUses(const SemanticInfo &semanticInfo, int revision)
{
    return semanticInfo.doc
        && semanticInfo.revision == static_cast<unsigned>(revision)
        && !semanticInfo.snapshot.isEmpty();
}

bool isMacroUseOf(const Document::MacroUse &marcoUse, const Macro &macro)
{
    const Macro &candidate = marcoUse.macro();

    return candidate.line() == macro.line()
        && candidate.utf16CharOffset() == macro.utf16CharOffset()
        && candidate.length() == macro.length()
        && candidate.fileName() == macro.fileName();
}

bool handleMacroCase(const Document::Ptr document,
                     const QTextCursor &textCursor,
                     CursorInfo::Ranges *ranges)
{
    QTC_ASSERT(ranges, return false);

    const Macro *macro = CppTools::findCanonicalMacro(textCursor, document);
    if (!macro)
        return false;

    const int length = macro->nameToQString().size();

    // Macro definition
    if (macro->fileName() == document->fileName())
        ranges->append(toRange(textCursor, macro->utf16CharOffset(), length));

    // Other macro uses
    foreach (const Document::MacroUse &use, document->macroUses()) {
        if (isMacroUseOf(use, *macro))
            ranges->append(toRange(textCursor, use.utf16charsBegin(), length));
    }

    return true;
}

} // anonymous namespace

QFuture<CursorInfo> BuiltinCursorInfo::run(const CursorInfoParams &cursorInfoParams)
{
    QFuture<CursorInfo> nothing;

    const SemanticInfo semanticInfo = cursorInfoParams.semanticInfo;
    const int currentDocumentRevision = cursorInfoParams.textCursor.document()->revision();
    if (!isSemanticInfoValidExceptLocalUses(semanticInfo, currentDocumentRevision))
        return nothing;

    const Document::Ptr document = semanticInfo.doc;
    const Snapshot snapshot = semanticInfo.snapshot;
    if (!document)
        return nothing;

    QTC_ASSERT(document->translationUnit(), return nothing);
    QTC_ASSERT(document->translationUnit()->ast(), return nothing);
    QTC_ASSERT(!snapshot.isEmpty(), return nothing);

    CursorInfo::Ranges ranges;
    if (handleMacroCase(document, cursorInfoParams.textCursor, &ranges)) {
        CursorInfo result;
        result.useRanges = ranges;
        result.areUseRangesForLocalVariable = false;

        QFutureInterface<CursorInfo> fi;
        fi.reportResult(result);
        fi.reportFinished();

        return fi.future();
    }

    const QTextCursor &textCursor = cursorInfoParams.textCursor;
    int line, column;
    Utils::Text::convertPosition(textCursor.document(), textCursor.position(), &line, &column);
    CanonicalSymbol canonicalSymbol(document, snapshot);
    QString expression;
    Scope *scope = canonicalSymbol.getScopeAndExpression(textCursor, &expression);

    return Utils::runAsync(&FindUses::find, document, snapshot, line, column, scope, expression);
}

CppTools::SemanticInfo::LocalUseMap
BuiltinCursorInfo::findLocalUses(const Document::Ptr &document, int line, int column)
{
    if (!document || !document->translationUnit() || !document->translationUnit()->ast())
        return SemanticInfo::LocalUseMap();

    AST *ast = document->translationUnit()->ast();
    FunctionDefinitionUnderCursor functionDefinitionUnderCursor(document->translationUnit());
    DeclarationAST *declaration = functionDefinitionUnderCursor(ast, line, column);
    return CppTools::LocalSymbols(document, declaration).uses;
}

} // namespace CppTools
