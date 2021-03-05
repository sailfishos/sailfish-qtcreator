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

#include "qmloutlinemodel.h"
#include "qmljseditor.h"

#include <qmljs/parser/qmljsastvisitor_p.h>
#include <qmljs/qmljscontext.h>
#include <qmljs/qmljsscopechain.h>
#include <qmljs/qmljsmodelmanagerinterface.h>
#include <qmljs/qmljsrewriter.h>
#include <qmljs/qmljsvalueowner.h>
#include <qmljstools/qmljsrefactoringchanges.h>

#include <utils/qtcassert.h>
#include <utils/dropsupport.h>

#include <coreplugin/icore.h>
#include <QDebug>
#include <QTime>
#include <QMimeData>
#include <typeinfo>

using namespace QmlJS;
using namespace QmlJSEditor::Internal;
using namespace QmlJSTools;

enum {
    debug = false
};

static const char INTERNAL_MIMETYPE[] = "application/x-qtcreator-qmloutlinemodel";

namespace QmlJSEditor {
namespace Internal {

QmlOutlineItem::QmlOutlineItem(QmlOutlineModel *model) :
    m_outlineModel(model)
{
}

QVariant QmlOutlineItem::data(int role) const
{
    if (role == Qt::ToolTipRole) {
        SourceLocation location = m_outlineModel->sourceLocation(index());
        AST::UiQualifiedId *uiQualifiedId = m_outlineModel->idNode(index());
        if (!uiQualifiedId || !location.isValid() || !m_outlineModel->m_semanticInfo.isValid())
            return QVariant();

        QList<AST::Node *> astPath = m_outlineModel->m_semanticInfo.rangePath(location.begin());
        ScopeChain scopeChain = m_outlineModel->m_semanticInfo.scopeChain(astPath);
        const Value *value = scopeChain.evaluate(uiQualifiedId);

        return prettyPrint(value, scopeChain.context());
    }

    if (role == Qt::DecorationRole)
        return m_outlineModel->icon(index());

    return QStandardItem::data(role);
}

int QmlOutlineItem::type() const
{
    return UserType;
}

void QmlOutlineItem::setItemData(const QMap<int, QVariant> &roles)
{
    QMap<int,QVariant>::const_iterator iter(roles.constBegin());
    while (iter != roles.constEnd()) {
        setData(iter.value(), iter.key());
        ++iter;
    }
}

QString QmlOutlineItem::prettyPrint(const Value *value, const ContextPtr &context) const
{
    if (! value)
        return QString();

    if (const ObjectValue *objectValue = value->asObjectValue()) {
        const QString className = objectValue->className();
        if (!className.isEmpty())
            return className;
    }

    const QString typeId = context->valueOwner()->typeId(value);
    if (typeId == QLatin1String("undefined"))
        return QString();

    return typeId;
}

/**
  Returns mapping of every UiObjectMember object to it's direct UiObjectMember parent object.
  */
class ObjectMemberParentVisitor : public AST::Visitor
{
public:
    QHash<AST::Node *,AST::UiObjectMember *> operator()(Document::Ptr doc) {
        parent.clear();
        if (doc && doc->ast())
            doc->ast()->accept(this);
        return parent;
    }

private:
    QHash<AST::Node *, AST::UiObjectMember *> parent;
    QList<AST::UiObjectMember *> stack;

    bool preVisit(AST::Node *node) override
    {
        if (AST::UiObjectMember *objMember = node->uiObjectMemberCast())
            stack.append(objMember);
        return true;
    }

    void postVisit(AST::Node *node) override
    {
        if (AST::UiObjectMember *objMember = node->uiObjectMemberCast()) {
            stack.removeLast();
            if (!stack.isEmpty())
                parent.insert(objMember, stack.last());
        } else if (AST::FunctionExpression *funcMember = node->asFunctionDefinition()) {
            if (!stack.isEmpty())
                parent.insert(funcMember, stack.last());
        }
    }

    void throwRecursionDepthError() override
    {
        qWarning("Warning: Hit maximum recursion depth while visiting AST in ObjectMemberParentVisitor");
    }
};


class QmlOutlineModelSync : protected AST::Visitor
{
public:
    QmlOutlineModelSync(QmlOutlineModel *model) :
        m_model(model),
        indent(0)
    {
    }

    void operator()(Document::Ptr doc)
    {
        m_nodeToIndex.clear();

        if (debug)
            qDebug() << "QmlOutlineModel ------";
        if (doc && doc->ast())
            doc->ast()->accept(this);
    }

private:
    bool preVisit(AST::Node *node) override
    {
        if (!node)
            return false;
        if (debug)
            qDebug() << "QmlOutlineModel -" << QByteArray(indent++, '-').constData() << node << typeid(*node).name();
        return true;
    }

    void postVisit(AST::Node *) override
    {
        indent--;
    }

    using ElementType = QPair<QString,QString>;
    bool visit(AST::UiObjectDefinition *objDef) override
    {
        QModelIndex index = m_model->enterObjectDefinition(objDef);
        m_nodeToIndex.insert(objDef, index);
        return true;
    }

    void endVisit(AST::UiObjectDefinition * /*objDef*/) override
    {
        m_model->leaveObjectDefiniton();
    }

    bool visit(AST::UiObjectBinding *objBinding) override
    {
        QModelIndex index = m_model->enterObjectBinding(objBinding);
        m_nodeToIndex.insert(objBinding, index);
        return true;
    }

    void endVisit(AST::UiObjectBinding * /*objBinding*/) override
    {
        m_model->leaveObjectBinding();
    }

    bool visit(AST::UiArrayBinding *arrayBinding) override
    {
        QModelIndex index = m_model->enterArrayBinding(arrayBinding);
        m_nodeToIndex.insert(arrayBinding, index);

        return true;
    }

    void endVisit(AST::UiArrayBinding * /*arrayBinding*/) override
    {
        m_model->leaveArrayBinding();
    }

    bool visit(AST::UiScriptBinding *scriptBinding) override
    {
        QModelIndex index = m_model->enterScriptBinding(scriptBinding);
        m_nodeToIndex.insert(scriptBinding, index);

        return true;
    }

    void endVisit(AST::UiScriptBinding * /*scriptBinding*/) override
    {
        m_model->leaveScriptBinding();
    }

    bool visit(AST::UiPublicMember *publicMember) override
    {
        QModelIndex index = m_model->enterPublicMember(publicMember);
        m_nodeToIndex.insert(publicMember, index);

        return true;
    }

    void endVisit(AST::UiPublicMember * /*publicMember*/) override
    {
        m_model->leavePublicMember();
    }

    bool visit(AST::FunctionDeclaration *functionDeclaration) override
    {
        QModelIndex index = m_model->enterFunctionDeclaration(functionDeclaration);
        m_nodeToIndex.insert(functionDeclaration, index);

        return true;
    }

    void endVisit(AST::FunctionDeclaration * /*functionDeclaration*/) override
    {
        m_model->leaveFunctionDeclaration();
    }

    bool visit(AST::BinaryExpression *binExp) override
    {
        auto lhsIdent = AST::cast<const AST::IdentifierExpression *>(binExp->left);
        auto rhsObjLit = AST::cast<AST::ObjectPattern *>(binExp->right);

        if (lhsIdent && rhsObjLit && (lhsIdent->name == QLatin1String("testcase"))
            && (binExp->op == QSOperator::Assign)) {
            QModelIndex index = m_model->enterTestCase(rhsObjLit);
            m_nodeToIndex.insert(rhsObjLit, index);

            if (AST::PatternPropertyList *properties = rhsObjLit->properties)
                visitProperties(properties);

            m_model->leaveTestCase();
            return true;
        }

        // Collect method assignments for prototypes and objects and show as functions
        auto lhsField = AST::cast<AST::FieldMemberExpression *>(binExp->left);
        auto rhsFuncExpr = AST::cast<AST::FunctionExpression *>(binExp->right);

        if (lhsField && rhsFuncExpr && rhsFuncExpr->body && (binExp->op == QSOperator::Assign)) {
            QModelIndex index = m_model->enterFieldMemberExpression(lhsField, rhsFuncExpr);
            m_nodeToIndex.insert(lhsField, index);
            m_model->leaveFieldMemberExpression();
        }

        return true;
    }

    void visitProperties(AST::PatternPropertyList *properties)
    {
        while (properties) {
            QModelIndex index = m_model->enterTestCaseProperties(properties);
            m_nodeToIndex.insert(properties, index);
            if (auto assignment = AST::cast<const AST::PatternProperty *>(properties->property))
                if (auto objLiteral = AST::cast<const AST::ObjectPattern *>(assignment->initializer))
                    visitProperties(objLiteral->properties);

            m_model->leaveTestCaseProperties();
            properties = properties->next;
        }
    }

    void throwRecursionDepthError() override
    {
        qWarning("Warning: Hit maximum recursion limit visiting AST in QmlOutlineModelSync");
    }

    QmlOutlineModel *m_model;

    QHash<AST::Node*, QModelIndex> m_nodeToIndex;
    int indent;
};

QmlOutlineModel::QmlOutlineModel(QmlJSEditorDocument *document) :
    QStandardItemModel(document),
    m_editorDocument(document)
{
    m_icons = Icons::instance();
    const QString resourcePath = Core::ICore::resourcePath();
    Icons::instance()->setIconFilesPath(resourcePath + QLatin1String("/qmlicons"));

    setItemPrototype(new QmlOutlineItem(this));
}

QStringList QmlOutlineModel::mimeTypes() const
{
    QStringList types;
    types << QLatin1String(INTERNAL_MIMETYPE);
    types << Utils::DropSupport::mimeTypesForFilePaths();
    return types;
}


QMimeData *QmlOutlineModel::mimeData(const QModelIndexList &indexes) const
{
    if (indexes.isEmpty())
        return nullptr;
    auto data = new Utils::DropMimeData;
    data->setOverrideFileDropAction(Qt::CopyAction);
    QByteArray encoded;
    QDataStream stream(&encoded, QIODevice::WriteOnly);
    stream << indexes.size();

    for (const auto &index : indexes) {
        SourceLocation location = sourceLocation(index);
        data->addFile(m_editorDocument->filePath().toString(), location.startLine,
                      location.startColumn - 1 /*editors have 0-based column*/);

        QList<int> rowPath;
        for (QModelIndex i = index; i.isValid(); i = i.parent()) {
            rowPath.prepend(i.row());
        }

        stream << rowPath;
    }
    data->setData(QLatin1String(INTERNAL_MIMETYPE), encoded);
    return data;
}

bool QmlOutlineModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int /*column*/, const QModelIndex &parent)
{
    if (debug)
        qDebug() << __FUNCTION__ << row << parent;

    // check if the action is supported
    if (!data || !(action == Qt::CopyAction || action == Qt::MoveAction))
        return false;

    // We cannot reparent outside of the root item
    if (!parent.isValid())
        return false;

    // check if the format is supported
    QStringList types = mimeTypes();
    if (types.isEmpty())
        return false;
    QString format = types.at(0);
    if (!data->hasFormat(format))
        return false;

    // decode and insert
    QByteArray encoded = data->data(format);
    QDataStream stream(&encoded, QIODevice::ReadOnly);
    int indexSize;
    stream >> indexSize;
    QList<QmlOutlineItem*> itemsToMove;
    for (int i = 0; i < indexSize; ++i) {
        QList<int> rowPath;
        stream >> rowPath;

        QModelIndex index;
        foreach (int row, rowPath) {
            index = this->index(row, 0, index);
            if (!index.isValid())
                continue;
        }

        itemsToMove << static_cast<QmlOutlineItem*>(itemFromIndex(index));
    }

    auto targetItem = static_cast<QmlOutlineItem*>(itemFromIndex(parent));
    reparentNodes(targetItem, row, itemsToMove);

    // Prevent view from calling removeRow() on it's own
    return false;
}

Qt::ItemFlags QmlOutlineModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return QStandardItemModel::flags(index);

    Qt::ItemFlags flags = Qt::ItemIsSelectable|Qt::ItemIsEnabled;

    // only allow drag&drop if we're in sync
    if (m_semanticInfo.isValid()
            && !m_editorDocument->isSemanticInfoOutdated()) {
        if (index.parent().isValid())
            flags |= Qt::ItemIsDragEnabled;
        if (index.data(ItemTypeRole) != NonElementBindingType)
            flags |= Qt::ItemIsDropEnabled;
    }
    return flags;
}

Qt::DropActions QmlOutlineModel::supportedDragActions() const
{
    // copy action used for dragging onto editor splits
    return Qt::MoveAction | Qt::CopyAction;
}

Qt::DropActions QmlOutlineModel::supportedDropActions() const
{
    return Qt::MoveAction;
}


Document::Ptr QmlOutlineModel::document() const
{
    return m_semanticInfo.document;
}

void QmlOutlineModel::update(const SemanticInfo &semanticInfo)
{
    m_semanticInfo = semanticInfo;
    if (! m_semanticInfo.isValid())
        return;

    m_treePos.clear();
    m_treePos.append(0);
    m_currentItem = invisibleRootItem();

    // resetModel() actually gives better average performance
    // then the incremental updates.
    beginResetModel();

    m_typeToIcon.clear();
    m_itemToNode.clear();
    m_itemToIdNode.clear();
    m_itemToIcon.clear();


    QmlOutlineModelSync syncModel(this);
    syncModel(m_semanticInfo.document);

    endResetModel();

    emit updated();
}

QModelIndex QmlOutlineModel::enterObjectDefinition(AST::UiObjectDefinition *objDef)
{
    const QString typeName = asString(objDef->qualifiedTypeNameId);

    QMap<int, QVariant> data;
    AST::UiQualifiedId *idNode = nullptr;
    QIcon icon;

    data.insert(Qt::DisplayRole, typeName);

    if (typeName.at(0).isUpper()) {
        data.insert(ItemTypeRole, ElementType);
        data.insert(AnnotationRole, getAnnotation(objDef->initializer));
        idNode = objDef->qualifiedTypeNameId;
        if (!m_typeToIcon.contains(typeName))
            m_typeToIcon.insert(typeName, getIcon(objDef->qualifiedTypeNameId));
        icon = m_typeToIcon.value(typeName);
    } else {
        // it's a grouped propery like 'anchors'
        data.insert(ItemTypeRole, NonElementBindingType);
        data.insert(AnnotationRole, QString()); // clear possible former annotation
        icon = Icons::scriptBindingIcon();
    }

    QmlOutlineItem *item = enterNode(data, objDef, idNode, icon);

    return item->index();
}

void QmlOutlineModel::leaveObjectDefiniton()
{
    leaveNode();
}

QModelIndex QmlOutlineModel::enterObjectBinding(AST::UiObjectBinding *objBinding)
{
    QMap<int, QVariant> bindingData;

    bindingData.insert(Qt::DisplayRole, asString(objBinding->qualifiedId));
    bindingData.insert(ItemTypeRole, ElementBindingType);
    bindingData.insert(AnnotationRole, QString()); // clear possible former annotation

    QmlOutlineItem *bindingItem = enterNode(bindingData, objBinding, objBinding->qualifiedId, Icons::scriptBindingIcon());

    const QString typeName = asString(objBinding->qualifiedTypeNameId);
    if (!m_typeToIcon.contains(typeName))
        m_typeToIcon.insert(typeName, getIcon(objBinding->qualifiedTypeNameId));

    QMap<int, QVariant> objectData;
    objectData.insert(Qt::DisplayRole, typeName);
    objectData.insert(AnnotationRole, getAnnotation(objBinding->initializer));
    objectData.insert(ItemTypeRole, ElementType);

    enterNode(objectData, objBinding, objBinding->qualifiedTypeNameId, m_typeToIcon.value(typeName));

    return bindingItem->index();
}

void QmlOutlineModel::leaveObjectBinding()
{
    leaveNode();
    leaveNode();
}

QModelIndex QmlOutlineModel::enterArrayBinding(AST::UiArrayBinding *arrayBinding)
{
    QMap<int, QVariant> bindingData;

    bindingData.insert(Qt::DisplayRole, asString(arrayBinding->qualifiedId));
    bindingData.insert(ItemTypeRole, ElementBindingType);
    bindingData.insert(AnnotationRole, QString()); // clear possible former annotation

    QmlOutlineItem *item = enterNode(bindingData, arrayBinding, arrayBinding->qualifiedId, Icons::scriptBindingIcon());

    return item->index();
}

void QmlOutlineModel::leaveArrayBinding()
{
    leaveNode();
}

QModelIndex QmlOutlineModel::enterScriptBinding(AST::UiScriptBinding *scriptBinding)
{
    QMap<int, QVariant> objectData;

    objectData.insert(Qt::DisplayRole, asString(scriptBinding->qualifiedId));
    objectData.insert(AnnotationRole, getAnnotation(scriptBinding->statement));
    objectData.insert(ItemTypeRole, NonElementBindingType);

    QmlOutlineItem *item = enterNode(objectData, scriptBinding, scriptBinding->qualifiedId, Icons::scriptBindingIcon());

    return item->index();
}

void QmlOutlineModel::leaveScriptBinding()
{
    leaveNode();
}

QModelIndex QmlOutlineModel::enterPublicMember(AST::UiPublicMember *publicMember)
{
    QMap<int, QVariant> objectData;

    if (!publicMember->name.isEmpty())
        objectData.insert(Qt::DisplayRole, publicMember->name.toString());
    objectData.insert(AnnotationRole, getAnnotation(publicMember->statement));
    objectData.insert(ItemTypeRole, NonElementBindingType);

    QmlOutlineItem *item = enterNode(objectData, publicMember, nullptr, Icons::publicMemberIcon());

    return item->index();
}

void QmlOutlineModel::leavePublicMember()
{
    leaveNode();
}

static QString functionDisplayName(QStringView name, AST::FormalParameterList *formals)
{
    QString display;

    if (!name.isEmpty())
        display += name.toString() + QLatin1Char('(');
    for (AST::FormalParameterList *param = formals; param; param = param->next) {
        display += param->element->bindingIdentifier.toString();
        if (param->next)
            display += QLatin1String(", ");
    }
    if (!name.isEmpty())
        display += QLatin1Char(')');

    return display;
}

QModelIndex QmlOutlineModel::enterFunctionDeclaration(AST::FunctionDeclaration *functionDeclaration)
{
    QMap<int, QVariant> objectData;

    objectData.insert(Qt::DisplayRole, functionDisplayName(functionDeclaration->name,
                                                           functionDeclaration->formals));
    objectData.insert(ItemTypeRole, ElementBindingType);
    objectData.insert(AnnotationRole, QString()); // clear possible former annotation

    QmlOutlineItem *item = enterNode(objectData, functionDeclaration, nullptr,
                                     Icons::functionDeclarationIcon());

    return item->index();
}

void QmlOutlineModel::leaveFunctionDeclaration()
{
    leaveNode();
}

QModelIndex QmlOutlineModel::enterFieldMemberExpression(AST::FieldMemberExpression *expression,
                                                        AST::FunctionExpression *functionExpression)
{
    QMap<int, QVariant> objectData;

    QString display = functionDisplayName(expression->name, functionExpression->formals);
    while (expression) {
        if (auto field = AST::cast<AST::FieldMemberExpression *>(expression->base)) {
            display.prepend(field->name.toString() + QLatin1Char('.'));
            expression = field;
        } else {
            if (auto ident = AST::cast<AST::IdentifierExpression *>(expression->base))
                display.prepend(ident->name.toString() + QLatin1Char('.'));
            break;
        }
    }

    objectData.insert(Qt::DisplayRole, display);
    objectData.insert(ItemTypeRole, ElementBindingType);
    objectData.insert(AnnotationRole, QString()); // clear possible former annotation

    QmlOutlineItem *item = enterNode(objectData, expression, nullptr,
                                     m_icons->functionDeclarationIcon());

    return item->index();
}

void QmlOutlineModel::leaveFieldMemberExpression()
{
    leaveNode();
}

QModelIndex QmlOutlineModel::enterTestCase(AST::ObjectPattern *objectLiteral)
{
    QMap<int, QVariant> objectData;

    objectData.insert(Qt::DisplayRole, QLatin1String("testcase"));
    objectData.insert(ItemTypeRole, ElementBindingType);
    objectData.insert(AnnotationRole, QString()); // clear possible former annotation

    QmlOutlineItem *item = enterNode(objectData, objectLiteral, nullptr,
                                     Icons::objectDefinitionIcon());

    return item->index();
}

void QmlOutlineModel::leaveTestCase()
{
    leaveNode();
}

QModelIndex QmlOutlineModel::enterTestCaseProperties(AST::PatternPropertyList *propertyAssignmentList)
{
    QMap<int, QVariant> objectData;
    if (auto assignment = AST::cast<AST::PatternProperty *>(
                propertyAssignmentList->property)) {
        if (auto propertyName = AST::cast<const AST::IdentifierPropertyName *>(assignment->name)) {
            objectData.insert(Qt::DisplayRole, propertyName->id.toString());
            objectData.insert(ItemTypeRole, ElementBindingType);
            objectData.insert(AnnotationRole, QString()); // clear possible former annotation
            QmlOutlineItem *item;
            if (assignment->initializer->kind == AST::Node::Kind_FunctionExpression)
                item = enterNode(objectData, assignment, nullptr, Icons::functionDeclarationIcon());
            else if (assignment->initializer->kind == AST::Node::Kind_ObjectPattern)
                item = enterNode(objectData, assignment, nullptr, Icons::objectDefinitionIcon());
            else
                item = enterNode(objectData, assignment, nullptr, Icons::scriptBindingIcon());

            return item->index();
        }
    }
    if (auto getterSetter = AST::cast<AST::PatternProperty *>(
                propertyAssignmentList->property)) {
        if (auto propertyName = AST::cast<const AST::IdentifierPropertyName *>(getterSetter->name)) {
            objectData.insert(Qt::DisplayRole, propertyName->id.toString());
            objectData.insert(ItemTypeRole, ElementBindingType);
            objectData.insert(AnnotationRole, QString()); // clear possible former annotation
            QmlOutlineItem *item;
            item = enterNode(objectData, getterSetter, nullptr, Icons::functionDeclarationIcon());

            return item->index();

        }
    }
    return QModelIndex();
}

void QmlOutlineModel::leaveTestCaseProperties()
{
    leaveNode();
}

AST::Node *QmlOutlineModel::nodeForIndex(const QModelIndex &index) const
{
    QTC_ASSERT(index.isValid() && (index.model() == this), return nullptr);
    if (index.isValid()) {
        auto item = static_cast<QmlOutlineItem*>(itemFromIndex(index));
        QTC_ASSERT(item, return nullptr);
        QTC_ASSERT(m_itemToNode.contains(item), return nullptr);
        return m_itemToNode.value(item);
    }
    return nullptr;
}

SourceLocation QmlOutlineModel::sourceLocation(const QModelIndex &index) const
{
    SourceLocation location;
    QTC_ASSERT(index.isValid() && (index.model() == this), return location);
    AST::Node *node = nodeForIndex(index);
    if (node) {
        if (AST::UiObjectMember *member = node->uiObjectMemberCast())
            location = getLocation(member);
        else if (AST::ExpressionNode *expression = node->expressionCast())
            location = getLocation(expression);
        else if (auto propertyAssignmentList = AST::cast<AST::PatternPropertyList *>(node))
            location = getLocation(propertyAssignmentList);
    }
    return location;
}

AST::UiQualifiedId *QmlOutlineModel::idNode(const QModelIndex &index) const
{
    QTC_ASSERT(index.isValid() && (index.model() == this), return nullptr);
    auto item = static_cast<QmlOutlineItem*>(itemFromIndex(index));
    return m_itemToIdNode.value(item);
}

QIcon QmlOutlineModel::icon(const QModelIndex &index) const
{
    QTC_ASSERT(index.isValid() && (index.model() == this), return QIcon());
    auto item = static_cast<QmlOutlineItem*>(itemFromIndex(index));
    return m_itemToIcon.value(item);
}

QmlOutlineItem *QmlOutlineModel::enterNode(QMap<int, QVariant> data, AST::Node *node, AST::UiQualifiedId *idNode, const QIcon &icon)
{
    int siblingIndex = m_treePos.last();
    QmlOutlineItem *newItem = nullptr;
    if (siblingIndex == 0) {
        // first child
        if (!m_currentItem->hasChildren()) {
            if (debug)
                qDebug() << "QmlOutlineModel - Adding" << "element to" << m_currentItem->text();

            newItem = new QmlOutlineItem(this);
        } else {
            m_currentItem = m_currentItem->child(0);
        }
    } else {
        // sibling
        if (m_currentItem->rowCount() <= siblingIndex) {
            if (debug)
                qDebug() << "QmlOutlineModel - Adding" << "element to" << m_currentItem->text();

            newItem = new QmlOutlineItem(this);
        } else {
            m_currentItem = m_currentItem->child(siblingIndex);
        }
    }

    QmlOutlineItem *item = newItem ? newItem : static_cast<QmlOutlineItem*>(m_currentItem);
    m_itemToNode.insert(item, node);
    m_itemToIdNode.insert(item, idNode);
    m_itemToIcon.insert(item, icon);

    if (newItem) {
        m_currentItem->appendRow(newItem);
        m_currentItem = newItem;
    }

    setItemData(m_currentItem->index(), data);

    m_treePos.append(0);

    return item;
}

void QmlOutlineModel::leaveNode()
{
    int lastIndex = m_treePos.takeLast();


    if (lastIndex > 0) {
        // element has children
        if (lastIndex < m_currentItem->rowCount()) {
            if (debug)
                qDebug() << "QmlOutlineModel - removeRows from " << m_currentItem->text() << lastIndex << m_currentItem->rowCount() - lastIndex;
            m_currentItem->removeRows(lastIndex, m_currentItem->rowCount() - lastIndex);
        }
        m_currentItem = parentItem();
    } else {
        if (m_currentItem->hasChildren()) {
            if (debug)
                qDebug() << "QmlOutlineModel - removeRows from " << m_currentItem->text() << 0 << m_currentItem->rowCount();
            m_currentItem->removeRows(0, m_currentItem->rowCount());
        }
        m_currentItem = parentItem();
    }


    m_treePos.last()++;
}

void QmlOutlineModel::reparentNodes(QmlOutlineItem *targetItem, int row, QList<QmlOutlineItem*> itemsToMove)
{
    Utils::ChangeSet changeSet;

    AST::UiObjectMember *targetObjectMember = m_itemToNode.value(targetItem)->uiObjectMemberCast();
    if (!targetObjectMember)
        return;

    QList<Utils::ChangeSet::Range> changedRanges;

    for (auto outlineItem : itemsToMove) {
        AST::UiObjectMember *sourceObjectMember = m_itemToNode.value(outlineItem)->uiObjectMemberCast();
        AST::FunctionExpression *functionMember = nullptr;
        if (!sourceObjectMember) {
            functionMember = m_itemToNode.value(outlineItem)->asFunctionDefinition();
            if (!functionMember)
                return;
        }

        m_itemToNode.value(outlineItem)->asFunctionDefinition();
        bool insertionOrderSpecified = true;
        AST::UiObjectMember *memberToInsertAfter = nullptr;
        {
            if (row == -1) {
                insertionOrderSpecified = false;
            } else if (row > 0) {
                auto outlineItem = static_cast<QmlOutlineItem*>(targetItem->child(row - 1));
                memberToInsertAfter = m_itemToNode.value(outlineItem)->uiObjectMemberCast();
            }
        }

        Utils::ChangeSet::Range range;
        if (sourceObjectMember)
            moveObjectMember(sourceObjectMember, targetObjectMember, insertionOrderSpecified,
                             memberToInsertAfter, &changeSet, &range);
        else if (functionMember)
            moveObjectMember(functionMember, targetObjectMember, insertionOrderSpecified,
                             memberToInsertAfter, &changeSet, &range);
        changedRanges << range;
    }

    QmlJSRefactoringChanges refactoring(ModelManagerInterface::instance(), m_semanticInfo.snapshot);
    TextEditor::RefactoringFilePtr file = refactoring.file(m_semanticInfo.document->fileName());
    file->setChangeSet(changeSet);
    foreach (const Utils::ChangeSet::Range &range, changedRanges) {
        file->appendIndentRange(range);
    }
    file->apply();
}

void QmlOutlineModel::moveObjectMember(AST::Node *toMove,
                                       AST::UiObjectMember *newParent,
                                       bool insertionOrderSpecified,
                                       AST::UiObjectMember *insertAfter,
                                       Utils::ChangeSet *changeSet,
                                       Utils::ChangeSet::Range *addedRange)
{
    Q_ASSERT(toMove);
    Q_ASSERT(newParent);
    Q_ASSERT(changeSet);

    QHash<AST::Node *, AST::UiObjectMember *> parentMembers;
    {
        ObjectMemberParentVisitor visitor;
        parentMembers = visitor(m_semanticInfo.document);
    }

    AST::UiObjectMember *oldParent = parentMembers.value(toMove);
    Q_ASSERT(oldParent);

    // make sure that target parent is actually a direct ancestor of target sibling
    if (insertAfter) {
        auto objMember = parentMembers.value(insertAfter);
        Q_ASSERT(objMember);
        newParent = objMember;
    }

    const QString documentText = m_semanticInfo.document->source();

    Rewriter rewriter(documentText, changeSet, QStringList());

    if (auto objDefinition = AST::cast<const AST::UiObjectDefinition*>(newParent)) {
        AST::UiObjectMemberList *listInsertAfter = nullptr;
        if (insertionOrderSpecified) {
            if (insertAfter) {
                listInsertAfter = objDefinition->initializer->members;
                while (listInsertAfter && (listInsertAfter->member != insertAfter))
                    listInsertAfter = listInsertAfter->next;
            }
        }

        if (auto moveScriptBinding = AST::cast<const AST::UiScriptBinding*>(toMove)) {
            const QString propertyName = asString(moveScriptBinding->qualifiedId);
            QString propertyValue;
            {
                const int offset = moveScriptBinding->statement->firstSourceLocation().begin();
                const int length = moveScriptBinding->statement->lastSourceLocation().end() - offset;
                propertyValue = documentText.mid(offset, length);
            }
            Rewriter::BindingType bindingType = Rewriter::ScriptBinding;

            if (insertionOrderSpecified)
                *addedRange = rewriter.addBinding(objDefinition->initializer, propertyName, propertyValue, bindingType, listInsertAfter);
            else
                *addedRange = rewriter.addBinding(objDefinition->initializer, propertyName, propertyValue, bindingType);
        } else {
            QString strToMove;
            {
                const int offset = toMove->firstSourceLocation().begin();
                const int length = toMove->lastSourceLocation().end() - offset;
                strToMove = documentText.mid(offset, length);
            }

            if (insertionOrderSpecified)
                *addedRange = rewriter.addObject(objDefinition->initializer, strToMove, listInsertAfter);
            else
                *addedRange = rewriter.addObject(objDefinition->initializer, strToMove);
        }
    } else if (auto arrayBinding = AST::cast<AST::UiArrayBinding*>(newParent)) {
        AST::UiArrayMemberList *listInsertAfter = nullptr;
        if (insertionOrderSpecified) {
            if (insertAfter) {
                listInsertAfter = arrayBinding->members;
                while (listInsertAfter && (listInsertAfter->member != insertAfter))
                    listInsertAfter = listInsertAfter->next;
            }
        }
        QString strToMove;
        {
            const int offset = toMove->firstSourceLocation().begin();
            const int length = toMove->lastSourceLocation().end() - offset;
            strToMove = documentText.mid(offset, length);
        }

        if (insertionOrderSpecified)
            *addedRange = rewriter.addObject(arrayBinding, strToMove, listInsertAfter);
        else
            *addedRange = rewriter.addObject(arrayBinding, strToMove);
    } else if (AST::cast<AST::UiObjectBinding*>(newParent)) {
        qDebug() << "TODO: Reparent to UiObjectBinding";
        return;
        // target is a property
    } else {
        return;
    }

    rewriter.removeObjectMember(toMove, oldParent);
}

QStandardItem *QmlOutlineModel::parentItem()
{
    QStandardItem *parent = m_currentItem->parent();
    if (!parent)
        parent = invisibleRootItem();
    return parent;
}


QString QmlOutlineModel::asString(AST::UiQualifiedId *id)
{
    QString text;
    for (; id; id = id->next) {
        if (!id->name.isEmpty())
            text += id->name.toString();
        else
            text += QLatin1Char('?');

        if (id->next)
            text += QLatin1Char('.');
    }

    return text;
}

SourceLocation QmlOutlineModel::getLocation(AST::UiObjectMember *objMember) {
    SourceLocation location;
    location = objMember->firstSourceLocation();
    location.length = objMember->lastSourceLocation().offset
            - objMember->firstSourceLocation().offset
            + objMember->lastSourceLocation().length;
    return location;
}

SourceLocation QmlOutlineModel::getLocation(AST::ExpressionNode *exprNode) {
    SourceLocation location;
    location = exprNode->firstSourceLocation();
    location.length = exprNode->lastSourceLocation().offset
            - exprNode->firstSourceLocation().offset
            + exprNode->lastSourceLocation().length;
    return location;
}

SourceLocation QmlOutlineModel::getLocation(AST::PatternPropertyList *propertyNode) {
    if (auto assignment = AST::cast<AST::PatternProperty *>(propertyNode->property))
        return getLocation(assignment);
    return propertyNode->firstSourceLocation(); // should never happen
}

SourceLocation QmlOutlineModel::getLocation(AST::PatternProperty *propertyNode) {
    SourceLocation location;
    location = propertyNode->name->propertyNameToken;
    location.length = propertyNode->initializer->lastSourceLocation().end() - location.offset;

    return location;
}

QIcon QmlOutlineModel::getIcon(AST::UiQualifiedId *qualifiedId) {
    QIcon icon;
    if (qualifiedId) {
        QString name = asString(qualifiedId);
        if (name.contains(QLatin1Char('.')))
            name = name.split(QLatin1Char('.')).last();

        // TODO: get rid of namespace prefixes.
        icon = m_icons->icon(QLatin1String("Qt"), name);
        if (icon.isNull())
            icon = m_icons->icon(QLatin1String("QtWebkit"), name);
    }
    return icon;
}

QString QmlOutlineModel::getAnnotation(AST::UiObjectInitializer *objectInitializer) {
    const QHash<QString,QString> bindings = getScriptBindings(objectInitializer);

    if (bindings.contains(QLatin1String("id")))
        return bindings.value(QLatin1String("id"));

    if (bindings.contains(QLatin1String("name")))
        return bindings.value(QLatin1String("name"));

    if (bindings.contains(QLatin1String("target")))
        return bindings.value(QLatin1String("target"));

    return QString();
}

QString QmlOutlineModel::getAnnotation(AST::Statement *statement)
{
    if (auto expr = AST::cast<const AST::ExpressionStatement*>(statement))
        return getAnnotation(expr->expression);
    return QString();
}

QString QmlOutlineModel::getAnnotation(AST::ExpressionNode *expression)
{
    if (!expression)
        return QString();
    QString source = m_semanticInfo.document->source();
    QString str = source.mid(expression->firstSourceLocation().begin(),
                             expression->lastSourceLocation().end()
                             - expression->firstSourceLocation().begin());
    // only show first line
    return str.left(str.indexOf(QLatin1Char('\n')));
}

QHash<QString,QString> QmlOutlineModel::getScriptBindings(AST::UiObjectInitializer *objectInitializer) {
    QHash <QString,QString> scriptBindings;
    for (AST::UiObjectMemberList *it = objectInitializer->members; it; it = it->next) {
        if (auto binding = AST::cast<const AST::UiScriptBinding*>(it->member)) {
            const QString bindingName = asString(binding->qualifiedId);
            scriptBindings.insert(bindingName, getAnnotation(binding->statement));
        }
    }
    return scriptBindings;
}

} // namespace Internal
} // namespace QmlJSEditor
