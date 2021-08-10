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

#include "cppelementevaluator.h"

#include <cpptools/cppmodelmanager.h>
#include <cpptools/cpptoolsreuse.h>
#include <cpptools/symbolfinder.h>
#include <cpptools/typehierarchybuilder.h>

#include <texteditor/textdocument.h>

#include <cplusplus/ExpressionUnderCursor.h>
#include <cplusplus/Icons.h>
#include <cplusplus/TypeOfExpression.h>

#include <utils/runextensions.h>

#include <QDir>
#include <QSet>
#include <QQueue>

using namespace CPlusPlus;

namespace CppTools {

static QStringList stripName(const QString &name)
{
    QStringList all;
    all << name;
    int colonColon = 0;
    const int size = name.size();
    while ((colonColon = name.indexOf(QLatin1String("::"), colonColon)) != -1) {
        all << name.right(size - colonColon - 2);
        colonColon += 2;
    }
    return all;
}

CppElement::CppElement() = default;

CppElement::~CppElement() = default;

CppClass *CppElement::toCppClass()
{
    return nullptr;
}

class Unknown : public CppElement
{
public:
    explicit Unknown(const QString &type) : type(type)
    {
        tooltip = type;
    }

public:
    QString type;
};

class CppInclude : public CppElement
{
public:
    explicit CppInclude(const Document::Include &includeFile)
        : path(QDir::toNativeSeparators(includeFile.resolvedFileName()))
        , fileName(Utils::FilePath::fromString(includeFile.resolvedFileName()).fileName())
    {
        helpCategory = Core::HelpItem::Brief;
        helpIdCandidates = QStringList(fileName);
        helpMark = fileName;
        link = Utils::Link(path);
        tooltip = path;
    }

public:
    QString path;
    QString fileName;
};

class CppMacro : public CppElement
{
public:
    explicit CppMacro(const Macro &macro)
    {
        helpCategory = Core::HelpItem::Macro;
        const QString macroName = QString::fromUtf8(macro.name(), macro.name().size());
        helpIdCandidates = QStringList(macroName);
        helpMark = macroName;
        link = Utils::Link(macro.fileName(), macro.line());
        tooltip = macro.toStringWithLineBreaks();
    }
};

// CppDeclarableElement
CppDeclarableElement::CppDeclarableElement(Symbol *declaration)
    : CppElement()
    , declaration(declaration)
    , icon(Icons::iconForSymbol(declaration))
{
    Overview overview;
    overview.showArgumentNames = true;
    overview.showReturnTypes = true;
    overview.showTemplateParameters = true;
    name = overview.prettyName(declaration->name());
    if (declaration->enclosingScope()->isClass() ||
        declaration->enclosingScope()->isNamespace() ||
        declaration->enclosingScope()->isEnum() ||
        declaration->enclosingScope()->isTemplate()) {
        qualifiedName = overview.prettyName(LookupContext::fullyQualifiedName(declaration));
        helpIdCandidates = stripName(qualifiedName);
    } else {
        qualifiedName = name;
        helpIdCandidates.append(name);
    }

    tooltip = overview.prettyType(declaration->type(), qualifiedName);
    link = declaration->toLink();
    helpMark = name;
}

class CppNamespace : public CppDeclarableElement
{
public:
    explicit CppNamespace(Symbol *declaration)
        : CppDeclarableElement(declaration)
    {
        helpCategory = Core::HelpItem::ClassOrNamespace;
        tooltip = qualifiedName;
    }
};

CppClass::CppClass(Symbol *declaration) : CppDeclarableElement(declaration)
{
    helpCategory = Core::HelpItem::ClassOrNamespace;
    tooltip = qualifiedName;
}

bool CppClass::operator==(const CppClass &other)
{
    return this->declaration == other.declaration;
}

CppClass *CppClass::toCppClass()
{
    return this;
}

void CppClass::lookupBases(QFutureInterfaceBase &futureInterface,
                           Symbol *declaration, const LookupContext &context)
{
    using Data = QPair<ClassOrNamespace*, CppClass*>;

    if (ClassOrNamespace *clazz = context.lookupType(declaration)) {
        QSet<ClassOrNamespace *> visited;

        QQueue<Data> q;
        q.enqueue(qMakePair(clazz, this));
        while (!q.isEmpty()) {
            if (futureInterface.isCanceled())
                return;
            Data current = q.dequeue();
            clazz = current.first;
            visited.insert(clazz);
            const QList<ClassOrNamespace *> &bases = clazz->usings();
            foreach (ClassOrNamespace *baseClass, bases) {
                const QList<Symbol *> &symbols = baseClass->symbols();
                foreach (Symbol *symbol, symbols) {
                    if (symbol->isClass() && (
                        clazz = context.lookupType(symbol)) &&
                        !visited.contains(clazz)) {
                        CppClass baseCppClass(symbol);
                        CppClass *cppClass = current.second;
                        cppClass->bases.append(baseCppClass);
                        q.enqueue(qMakePair(clazz, &cppClass->bases.last()));
                    }
                }
            }
        }
    }
}

void CppClass::lookupDerived(QFutureInterfaceBase &futureInterface,
                             Symbol *declaration, const Snapshot &snapshot)
{
    using Data = QPair<CppClass*, TypeHierarchy>;

    snapshot.updateDependencyTable(futureInterface);
    if (futureInterface.isCanceled())
        return;
    const TypeHierarchy &completeHierarchy
            = TypeHierarchyBuilder::buildDerivedTypeHierarchy(futureInterface, declaration, snapshot);

    QQueue<Data> q;
    q.enqueue(qMakePair(this, completeHierarchy));
    while (!q.isEmpty()) {
        if (futureInterface.isCanceled())
            return;
        const Data &current = q.dequeue();
        CppClass *clazz = current.first;
        const TypeHierarchy &classHierarchy = current.second;
        foreach (const TypeHierarchy &derivedHierarchy, classHierarchy.hierarchy()) {
            clazz->derived.append(CppClass(derivedHierarchy.symbol()));
            q.enqueue(qMakePair(&clazz->derived.last(), derivedHierarchy));
        }
    }
}

class CppFunction : public CppDeclarableElement
{
public:
    explicit CppFunction(Symbol *declaration)
        : CppDeclarableElement(declaration)
    {
        helpCategory = Core::HelpItem::Function;

        const FullySpecifiedType &type = declaration->type();

        // Functions marks can be found either by the main overload or signature based
        // (with no argument names and no return). Help ids have no signature at all.
        Overview overview;
        overview.showDefaultArguments = false;
        helpMark = overview.prettyType(type, name);

        overview.showFunctionSignatures = false;
        helpIdCandidates.append(overview.prettyName(declaration->name()));
    }
};

class CppEnum : public CppDeclarableElement
{
public:
    explicit CppEnum(Enum *declaration)
        : CppDeclarableElement(declaration)
    {
        helpCategory = Core::HelpItem::Enum;
        tooltip = qualifiedName;
    }
};

class CppTypedef : public CppDeclarableElement
{
public:
    explicit CppTypedef(Symbol *declaration)
        : CppDeclarableElement(declaration)
    {
        helpCategory = Core::HelpItem::Typedef;
        Overview overview;
        overview.showTemplateParameters = true;
        tooltip = overview.prettyType(declaration->type(), qualifiedName);
    }
};

class CppVariable : public CppDeclarableElement
{
public:
    explicit CppVariable(Symbol *declaration, const LookupContext &context, Scope *scope)
        : CppDeclarableElement(declaration)
    {
        const FullySpecifiedType &type = declaration->type();

        const Name *typeName = nullptr;
        if (type->isNamedType()) {
            typeName = type->asNamedType()->name();
        } else if (type->isPointerType() || type->isReferenceType()) {
            FullySpecifiedType associatedType;
            if (type->isPointerType())
                associatedType = type->asPointerType()->elementType();
            else
                associatedType = type->asReferenceType()->elementType();
            if (associatedType->isNamedType())
                typeName = associatedType->asNamedType()->name();
        }

        if (typeName) {
            if (ClassOrNamespace *clazz = context.lookupType(typeName, scope)) {
                if (!clazz->symbols().isEmpty()) {
                    Overview overview;
                    Symbol *symbol = clazz->symbols().at(0);
                    const QString &name = overview.prettyName(
                        LookupContext::fullyQualifiedName(symbol));
                    if (!name.isEmpty()) {
                        tooltip = name;
                        helpCategory = Core::HelpItem::ClassOrNamespace;
                        const QStringList &allNames = stripName(name);
                        if (!allNames.isEmpty()) {
                            helpMark = allNames.last();
                            helpIdCandidates = allNames;
                        }
                    }
                }
            }
        }
    }
};

class CppEnumerator : public CppDeclarableElement
{
public:
    explicit CppEnumerator(EnumeratorDeclaration *declaration)
        : CppDeclarableElement(declaration)
    {
        helpCategory = Core::HelpItem::Enum;

        Overview overview;

        Symbol *enumSymbol = declaration->enclosingScope();
        const QString enumName = overview.prettyName(LookupContext::fullyQualifiedName(enumSymbol));
        const QString enumeratorName = overview.prettyName(declaration->name());
        QString enumeratorValue;
        if (const StringLiteral *value = declaration->constantValue())
            enumeratorValue = QString::fromUtf8(value->chars(), value->size());

        helpMark = overview.prettyName(enumSymbol->name());

        tooltip = enumeratorName;
        if (!enumName.isEmpty())
            tooltip.prepend(enumName + QLatin1Char(' '));
        if (!enumeratorValue.isEmpty())
            tooltip.append(QLatin1String(" = ") + enumeratorValue);
    }
};

static bool isCppClass(Symbol *symbol)
{
    return symbol->isClass() || symbol->isForwardClassDeclaration()
            || (symbol->isTemplate() && symbol->asTemplate()->declaration()
                && (symbol->asTemplate()->declaration()->isClass()
                    || symbol->asTemplate()->declaration()->isForwardClassDeclaration()));
}

static Symbol *followClassDeclaration(Symbol *symbol, const Snapshot &snapshot, SymbolFinder symbolFinder,
                               LookupContext *context = nullptr)
{
    if (!symbol->isForwardClassDeclaration())
        return symbol;

    Symbol *classDeclaration = symbolFinder.findMatchingClassDeclaration(symbol, snapshot);
    if (!classDeclaration)
        return symbol;

    if (context) {
        const QString fileName = QString::fromUtf8(classDeclaration->fileName(),
                                                   classDeclaration->fileNameLength());
        const Document::Ptr declarationDocument = snapshot.document(fileName);
        if (declarationDocument != context->thisDocument())
            (*context) = LookupContext(declarationDocument, snapshot);
    }
    return classDeclaration;
}

static Symbol *followTemplateAsClass(Symbol *symbol)
{
    if (Template *t = symbol->asTemplate(); t && t->declaration() && t->declaration()->asClass())
        return t->declaration()->asClass();
    return symbol;
}

static void createTypeHierarchy(QFutureInterface<QSharedPointer<CppElement>> &futureInterface,
                                const Snapshot &snapshot,
                                const LookupItem &lookupItem,
                                const LookupContext &context,
                                SymbolFinder symbolFinder)
{
    if (futureInterface.isCanceled())
        return;

    Symbol *declaration = lookupItem.declaration();
    if (!declaration)
        return;

    if (!isCppClass(declaration))
        return;

    LookupContext contextToUse = context;
    declaration = followClassDeclaration(declaration, snapshot, symbolFinder, &contextToUse);
    declaration = followTemplateAsClass(declaration);

    if (futureInterface.isCanceled())
        return;
    QSharedPointer<CppClass> cppClass(new CppClass(declaration));
    cppClass->lookupBases(futureInterface, declaration, contextToUse);
    if (futureInterface.isCanceled())
        return;
    cppClass->lookupDerived(futureInterface, declaration, snapshot);
    if (futureInterface.isCanceled())
        return;
    futureInterface.reportResult(cppClass);
}

static QSharedPointer<CppElement> handleLookupItemMatch(const Snapshot &snapshot,
                                                        const LookupItem &lookupItem,
                                                        const LookupContext &context,
                                                        SymbolFinder symbolFinder)
{
    QSharedPointer<CppElement> element;
    Symbol *declaration = lookupItem.declaration();
    if (!declaration) {
        const QString &type = Overview().prettyType(lookupItem.type(), QString());
        element = QSharedPointer<CppElement>(new Unknown(type));
    } else {
        const FullySpecifiedType &type = declaration->type();
        if (declaration->isNamespace()) {
            element = QSharedPointer<CppElement>(new CppNamespace(declaration));
        } else if (isCppClass(declaration)) {
            LookupContext contextToUse = context;
            declaration = followClassDeclaration(declaration, snapshot, symbolFinder, &contextToUse);
            element = QSharedPointer<CppElement>(new CppClass(declaration));
        } else if (Enum *enumDecl = declaration->asEnum()) {
            element = QSharedPointer<CppElement>(new CppEnum(enumDecl));
        } else if (auto enumerator = dynamic_cast<EnumeratorDeclaration *>(declaration)) {
            element = QSharedPointer<CppElement>(new CppEnumerator(enumerator));
        } else if (declaration->isTypedef()) {
            element = QSharedPointer<CppElement>(new CppTypedef(declaration));
        } else if (declaration->isFunction()
                   || (type.isValid() && type->isFunctionType())
                   || declaration->isTemplate()) {
            element = QSharedPointer<CppElement>(new CppFunction(declaration));
        } else if (declaration->isDeclaration() && type.isValid()) {
            element = QSharedPointer<CppElement>(
                new CppVariable(declaration, context, lookupItem.scope()));
        } else {
            element = QSharedPointer<CppElement>(new CppDeclarableElement(declaration));
        }
    }
    return element;
}

//  special case for bug QTCREATORBUG-4780
static bool shouldOmitElement(const LookupItem &lookupItem, const Scope *scope)
{
    return !lookupItem.declaration() && scope && scope->isFunction()
            && lookupItem.type().match(scope->asFunction()->returnType());
}

using namespace std::placeholders;
using ExecFunction = std::function<QFuture<QSharedPointer<CppElement>>
            (const CPlusPlus::Snapshot &, const CPlusPlus::LookupItem &,
             const CPlusPlus::LookupContext &)>;
using SourceFunction = std::function<bool(const CPlusPlus::Snapshot &,
                                          CPlusPlus::Document::Ptr &,
                                          CPlusPlus::Scope **, QString &)>;

static QFuture<QSharedPointer<CppElement>> createFinishedFuture()
{
    QFutureInterface<QSharedPointer<CppElement>> futureInterface;
    futureInterface.reportStarted();
    futureInterface.reportFinished();
    return futureInterface.future();
}

static LookupItem findLookupItem(const CPlusPlus::Snapshot &snapshot, CPlusPlus::Document::Ptr &doc,
       Scope *scope, const QString &expression, LookupContext *lookupContext, bool followTypedef)
{
    TypeOfExpression typeOfExpression;
    typeOfExpression.init(doc, snapshot);
    // make possible to instantiate templates
    typeOfExpression.setExpandTemplates(true);
    const QList<LookupItem> &lookupItems = typeOfExpression(expression.toUtf8(), scope);
    *lookupContext = typeOfExpression.context();
    if (lookupItems.isEmpty())
        return LookupItem();

    auto isInteresting = [followTypedef](Symbol *symbol) {
        return symbol && (!followTypedef || (symbol->isClass() || symbol->isTemplate()
               || symbol->isForwardClassDeclaration() || symbol->isTypedef()));
    };

    for (const LookupItem &item : lookupItems) {
        if (shouldOmitElement(item, scope))
            continue;
        Symbol *symbol = item.declaration();
        if (!isInteresting(symbol))
            continue;
        if (followTypedef && symbol->isTypedef()) {
            CPlusPlus::NamedType *namedType = symbol->type()->asNamedType();
            if (!namedType) {
                // Anonymous aggregate such as: typedef struct {} Empty;
                continue;
            }
            return TypeHierarchyBuilder::followTypedef(*lookupContext,
                         namedType->name(), symbol->enclosingScope());
        }
        return item;
    }
    return LookupItem();
}

static QFuture<QSharedPointer<CppElement>> exec(SourceFunction &&sourceFunction,
                                                ExecFunction &&execFunction,
                                                bool followTypedef = true)
{
    const Snapshot &snapshot = CppModelManager::instance()->snapshot();

    Document::Ptr doc;
    QString expression;
    Scope *scope = nullptr;
    if (!std::invoke(std::forward<SourceFunction>(sourceFunction), snapshot, doc, &scope, expression))
        return createFinishedFuture();

    LookupContext lookupContext;
    const LookupItem &lookupItem = findLookupItem(snapshot, doc, scope, expression, &lookupContext,
                                                  followTypedef);
    if (!lookupItem.declaration())
        return createFinishedFuture();

    return std::invoke(std::forward<ExecFunction>(execFunction), snapshot, lookupItem, lookupContext);
}

static QFuture<QSharedPointer<CppElement>> asyncExec(
        const CPlusPlus::Snapshot &snapshot, const CPlusPlus::LookupItem &lookupItem,
        const CPlusPlus::LookupContext &lookupContext)
{
    return Utils::runAsync(&createTypeHierarchy, snapshot, lookupItem, lookupContext,
                           *CppModelManager::instance()->symbolFinder());
}

class FromExpressionFunctor
{
public:
    FromExpressionFunctor(const QString &expression, const QString &fileName)
        : m_expression(expression)
        , m_fileName(fileName)
    {}

    bool operator()(const CPlusPlus::Snapshot &snapshot, Document::Ptr &doc, Scope **scope,
                    QString &expression)
    {
        doc = snapshot.document(m_fileName);
        if (doc.isNull())
            return false;

        expression = m_expression;

        // Fetch the expression's code
        *scope = doc->globalNamespace();
        return true;
    }
private:
    const QString m_expression;
    const QString m_fileName;
};

QFuture<QSharedPointer<CppElement>> CppElementEvaluator::asyncExecute(const QString &expression,
                                                                      const QString &fileName)
{
    return exec(FromExpressionFunctor(expression, fileName), asyncExec);
}

class FromGuiFunctor
{
public:
    FromGuiFunctor(TextEditor::TextEditorWidget *editor)
        : m_editor(editor)
        , m_tc(editor->textCursor())
    {}

    bool operator()(const CPlusPlus::Snapshot &snapshot, Document::Ptr &doc, Scope **scope,
                    QString &expression)
    {
        doc = snapshot.document(m_editor->textDocument()->filePath());
        if (!doc)
            return false;

        int line = 0;
        int column = 0;
        const int pos = m_tc.position();
        m_editor->convertPosition(pos, &line, &column);

        checkDiagnosticMessage(pos);

        if (matchIncludeFile(doc, line) || matchMacroInUse(doc, pos))
            return false;

        CppTools::moveCursorToEndOfIdentifier(&m_tc);
        ExpressionUnderCursor expressionUnderCursor(doc->languageFeatures());
        expression = expressionUnderCursor(m_tc);

        // Fetch the expression's code
        *scope = doc->scopeAt(line, column - 1);
        return true;
    }
    QFuture<QSharedPointer<CppElement>> syncExec(const CPlusPlus::Snapshot &,
                     const CPlusPlus::LookupItem &, const CPlusPlus::LookupContext &);

private:
    void checkDiagnosticMessage(int pos);
    bool matchIncludeFile(const CPlusPlus::Document::Ptr &document, int line);
    bool matchMacroInUse(const CPlusPlus::Document::Ptr &document, int pos);

public:
    void clear();

    TextEditor::TextEditorWidget *m_editor;
    QTextCursor m_tc;
    QSharedPointer<CppElement> m_element;
    QString m_diagnosis;
};

QFuture<QSharedPointer<CppElement>> FromGuiFunctor::syncExec(
        const CPlusPlus::Snapshot &snapshot, const CPlusPlus::LookupItem &lookupItem,
        const CPlusPlus::LookupContext &lookupContext)
{
    QFutureInterface<QSharedPointer<CppElement>> futureInterface;
    futureInterface.reportStarted();
    m_element = handleLookupItemMatch(snapshot, lookupItem, lookupContext,
                                      *CppModelManager::instance()->symbolFinder());
    futureInterface.reportResult(m_element);
    futureInterface.reportFinished();
    return futureInterface.future();
}

void FromGuiFunctor::checkDiagnosticMessage(int pos)
{
    foreach (const QTextEdit::ExtraSelection &sel,
             m_editor->extraSelections(TextEditor::TextEditorWidget::CodeWarningsSelection)) {
        if (pos >= sel.cursor.selectionStart() && pos <= sel.cursor.selectionEnd()) {
            m_diagnosis = sel.format.toolTip();
            break;
        }
    }
}

bool FromGuiFunctor::matchIncludeFile(const Document::Ptr &document, int line)
{
    const QList<Document::Include> &includes = document->resolvedIncludes();
    for (const Document::Include &includeFile : includes) {
        if (includeFile.line() == line) {
            m_element = QSharedPointer<CppElement>(new CppInclude(includeFile));
            return true;
        }
    }
    return false;
}

bool FromGuiFunctor::matchMacroInUse(const Document::Ptr &document, int pos)
{
    const QList<Document::MacroUse> macros = document->macroUses();
    for (const Document::MacroUse &use : macros) {
        if (use.containsUtf16charOffset(pos)) {
            const int begin = use.utf16charsBegin();
            if (pos < begin + use.macro().nameToQString().size()) {
                m_element = QSharedPointer<CppElement>(new CppMacro(use.macro()));
                return true;
            }
        }
    }
    return false;
}

void FromGuiFunctor::clear()
{
    m_element.clear();
    m_diagnosis.clear();
}

class CppElementEvaluatorPrivate
{
public:
    CppElementEvaluatorPrivate(TextEditor::TextEditorWidget *editor) : m_functor(editor) {}
    FromGuiFunctor m_functor;
};

CppElementEvaluator::CppElementEvaluator(TextEditor::TextEditorWidget *editor)
    : d(new CppElementEvaluatorPrivate(editor))
{}

CppElementEvaluator::~CppElementEvaluator()
{
    delete d;
}

void CppElementEvaluator::setTextCursor(const QTextCursor &tc)
{
    d->m_functor.m_tc = tc;
}

QFuture<QSharedPointer<CppElement>> CppElementEvaluator::asyncExecute(
        TextEditor::TextEditorWidget *editor)
{
    return exec(FromGuiFunctor(editor), asyncExec);
}

void CppElementEvaluator::execute()
{
    d->m_functor.clear();
    exec(std::ref(d->m_functor), std::bind(&FromGuiFunctor::syncExec, &d->m_functor, _1, _2, _3), false);
}

bool CppElementEvaluator::identifiedCppElement() const
{
    return !d->m_functor.m_element.isNull();
}

const QSharedPointer<CppElement> &CppElementEvaluator::cppElement() const
{
    return d->m_functor.m_element;
}

bool CppElementEvaluator::hasDiagnosis() const
{
    return !d->m_functor.m_diagnosis.isEmpty();
}

const QString &CppElementEvaluator::diagnosis() const
{
    return d->m_functor.m_diagnosis;
}

Utils::Link CppElementEvaluator::linkFromExpression(const QString &expression, const QString &fileName)
{
    const Snapshot &snapshot = CppModelManager::instance()->snapshot();
    Document::Ptr doc = snapshot.document(fileName);
    if (doc.isNull())
        return Utils::Link();
    Scope *scope = doc->globalNamespace();

    TypeOfExpression typeOfExpression;
    typeOfExpression.init(doc, snapshot);
    typeOfExpression.setExpandTemplates(true);
    const QList<LookupItem> &lookupItems = typeOfExpression(expression.toUtf8(), scope);
    if (lookupItems.isEmpty())
        return Utils::Link();

    for (const LookupItem &item : lookupItems) {
        Symbol *symbol = item.declaration();
        if (!symbol)
            continue;
        if (!symbol->isClass() && !symbol->isTemplate())
            continue;
        return symbol->toLink();
    }
    return Utils::Link();
}

} // namespace CppTools
