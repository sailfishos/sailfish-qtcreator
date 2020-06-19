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

#include "qmljssemanticinfo.h"

#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/qmljsscopebuilder.h>
#include <qmljs/qmljsscopechain.h>
#include <qmljs/parser/qmljsengine_p.h>

#include <QDebug>

using namespace QmlJS;
using namespace QmlJS::AST;

namespace QmlJSTools {

namespace {

// ### does not necessarily give the full AST path!
// intentionally does not contain lists like
// UiHeaderItemList, SourceElements, UiObjectMemberList
class AstPath: protected AST::Visitor
{
    QList<AST::Node *> _path;
    unsigned _offset;

public:
    QList<AST::Node *> operator()(AST::Node *node, unsigned offset)
    {
        _offset = offset;
        _path.clear();
        accept(node);
        return _path;
    }

protected:
    using AST::Visitor::visit;

    void accept(AST::Node *node)
    {
        if (node)
            node->accept(this);
    }

    bool containsOffset(SourceLocation start, SourceLocation end)
    {
        return _offset >= start.begin() && _offset <= end.end();
    }

    bool handle(AST::Node *ast,
                SourceLocation start, SourceLocation end,
                bool addToPath = true)
    {
        if (containsOffset(start, end)) {
            if (addToPath)
                _path.append(ast);
            return true;
        }
        return false;
    }

    template <class T>
    bool handleLocationAst(T *ast, bool addToPath = true)
    {
        return handle(ast, ast->firstSourceLocation(), ast->lastSourceLocation(), addToPath);
    }

    bool preVisit(AST::Node *node) override
    {
        if (Statement *stmt = node->statementCast())
            return handleLocationAst(stmt);
        else if (ExpressionNode *exp = node->expressionCast())
            return handleLocationAst(exp);
        else if (UiObjectMember *ui = node->uiObjectMemberCast())
            return handleLocationAst(ui);
        return true;
    }

    bool visit(AST::UiQualifiedId *ast) override
    {
        SourceLocation first = ast->identifierToken;
        SourceLocation last;
        for (AST::UiQualifiedId *it = ast; it; it = it->next)
            last = it->identifierToken;
        if (containsOffset(first, last))
            _path.append(ast);
        return false;
    }

    bool visit(AST::UiProgram *ast) override
    {
        _path.append(ast);
        return true;
    }

    bool visit(AST::Program *ast) override
    {
        _path.append(ast);
        return true;
    }

    bool visit(AST::UiImport *ast) override
    {
        return handleLocationAst(ast);
    }

    void throwRecursionDepthError() override
    {
        qWarning("Warning: Hit maximum recursion depth when visiting the AST in AstPath");
    }
};

} // anonmymous

AST::Node *SemanticInfo::rangeAt(int cursorPosition) const
{
    AST::Node *declaringMember = nullptr;

    for (int i = ranges.size() - 1; i != -1; --i) {
        const Range &range = ranges.at(i);

        if (range.begin.isNull() || range.end.isNull()) {
            continue;
        } else if (cursorPosition >= range.begin.position() && cursorPosition <= range.end.position()) {
            declaringMember = range.ast;
            break;
        }
    }

    return declaringMember;
}

// ### the name and behavior of this function is dubious
Node *SemanticInfo::declaringMemberNoProperties(int cursorPosition) const
{
    AST::Node *node = rangeAt(cursorPosition);

    if (auto objectDefinition = cast<const UiObjectDefinition*>(node)) {
        const QStringRef name = objectDefinition->qualifiedTypeNameId->name;
        if (!name.isEmpty() && name.at(0).isLower()) {
            QList<AST::Node *> path = rangePath(cursorPosition);
            if (path.size() > 1)
                return path.at(path.size() - 2);
        } else if (name.contains(QLatin1String("GradientStop"))) {
            QList<AST::Node *> path = rangePath(cursorPosition);
            if (path.size() > 2)
                return path.at(path.size() - 3);
        }
    } else if (auto objectBinding = cast<const UiObjectBinding*>(node)) {
        const QStringRef name = objectBinding->qualifiedTypeNameId->name;
        if (name.contains(QLatin1String("Gradient"))) {
            QList<AST::Node *> path = rangePath(cursorPosition);
            if (path.size() > 1)
                return path.at(path.size() - 2);
        }
    }

    return node;
}

QList<AST::Node *> SemanticInfo::rangePath(int cursorPosition) const
{
    QList<AST::Node *> path;

    foreach (const Range &range, ranges) {
        if (range.begin.isNull() || range.end.isNull())
            continue;
        else if (cursorPosition >= range.begin.position() && cursorPosition <= range.end.position())
            path += range.ast;
    }

    return path;
}

ScopeChain SemanticInfo::scopeChain(const QList<Node *> &path) const
{
    Q_ASSERT(m_rootScopeChain);

    if (path.isEmpty())
        return *m_rootScopeChain;

    ScopeChain scope = *m_rootScopeChain;
    ScopeBuilder builder(&scope);
    builder.push(path);
    return scope;
}

void SemanticInfo::setRootScopeChain(QSharedPointer<const ScopeChain> rootScopeChain)
{
    Q_ASSERT(m_rootScopeChain.isNull());
    m_rootScopeChain = rootScopeChain;
}

QList<AST::Node *> SemanticInfo::astPath(int pos) const
{
    QList<AST::Node *> result;
    if (! document)
        return result;

    AstPath astPath;
    return astPath(document->ast(), pos);
}

AST::Node *SemanticInfo::astNodeAt(int pos) const
{
    const QList<AST::Node *> path = astPath(pos);
    if (path.isEmpty())
        return nullptr;
    return path.last();
}

SemanticInfo::SemanticInfo(ScopeChain *rootScopeChain)
    : m_rootScopeChain(rootScopeChain)
{
}

bool SemanticInfo::isValid() const
{
    if (document && context && m_rootScopeChain)
        return true;

    return false;
}

int SemanticInfo::revision() const
{
    if (document)
        return document->editorRevision();

    return 0;
}

} // namespace QmlJSTools
