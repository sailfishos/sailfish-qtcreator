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

#include "qmldesignercorelib_global.h"
#include "exception.h"
#include "abstractview.h"
#include "documentmessage.h"

#include <QScopedPointer>
#include <QTimer>
#include <QUrl>

#include <functional>

namespace QmlJS {
class Document;
class ScopeChain;
}

namespace QmlDesigner {

class TextModifier;

namespace Internal {

class TextToModelMerger;
class ModelToTextMerger;
class ModelNodePositionStorage;

} //Internal

struct QmlTypeData
{
    QString superClassName;
    QString importUrl;
    QString versionString;
    QString cppClassName;
    QString typeName;
    bool isSingleton = false;
    bool isCppType = false;
};

class QMLDESIGNERCORE_EXPORT RewriterView : public AbstractView
{
    Q_OBJECT

public:
    enum DifferenceHandling {
        Validate,
        Amend
    };

public:
    RewriterView(DifferenceHandling differenceHandling = RewriterView::Amend, QObject *parent = nullptr);
    ~RewriterView() override;

    void modelAttached(Model *model) override;
    void modelAboutToBeDetached(Model *model) override;
    void nodeCreated(const ModelNode &createdNode) override;
    void nodeRemoved(const ModelNode &removedNode, const NodeAbstractProperty &parentProperty, PropertyChangeFlags propertyChange) override;
    void propertiesAboutToBeRemoved(const QList<AbstractProperty>& propertyList) override;
    void propertiesRemoved(const QList<AbstractProperty>& propertyList) override;
    void variantPropertiesChanged(const QList<VariantProperty>& propertyList, PropertyChangeFlags propertyChange) override;
    void bindingPropertiesChanged(const QList<BindingProperty>& propertyList, PropertyChangeFlags propertyChange) override;
    void signalHandlerPropertiesChanged(const QVector<SignalHandlerProperty>& propertyList,PropertyChangeFlags propertyChange) override;
    void nodeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent,
                        const NodeAbstractProperty &oldPropertyParent,
                        AbstractView::PropertyChangeFlags propertyChange) override;
    void nodeIdChanged(const ModelNode& node, const QString& newId, const QString& oldId) override;
    void nodeOrderChanged(const NodeListProperty &listProperty, const ModelNode &movedNode, int oldIndex) override;
    void rootNodeTypeChanged(const QString &type, int majorVersion, int minorVersion) override;
    void nodeTypeChanged(const ModelNode& node, const TypeName &type, int majorVersion, int minorVersion) override;
    void customNotification(const AbstractView *view, const QString &identifier,
                            const QList<ModelNode> &nodeList,
                            const QList<QVariant> &data) override;

    void rewriterBeginTransaction() override;
    void rewriterEndTransaction() override;

    void importsChanged(const QList<Import> &addedImports, const QList<Import> &removedImports) override;

    TextModifier *textModifier() const;
    void setTextModifier(TextModifier *textModifier);
    QString textModifierContent() const;

    void reactivateTextMofifierChangeSignals();
    void deactivateTextMofifierChangeSignals();

    void auxiliaryDataChanged(const ModelNode &node, const PropertyName &name, const QVariant &data) override;

    Internal::ModelNodePositionStorage *positionStorage() const;

    QList<DocumentMessage> warnings() const;
    QList<DocumentMessage> errors() const;
    void clearErrorAndWarnings();
    void setErrors(const QList<DocumentMessage> &errors);
    void setWarnings(const QList<DocumentMessage> &warnings);
    void setIncompleteTypeInformation(bool b);
    bool hasIncompleteTypeInformation() const;
    void addError(const DocumentMessage &error);

    void enterErrorState(const QString &errorMessage);
    bool inErrorState() const { return !m_rewritingErrorMessage.isEmpty(); }
    void leaveErrorState() { m_rewritingErrorMessage.clear(); }
    void resetToLastCorrectQml();

    QMap<ModelNode, QString> extractText(const QList<ModelNode> &nodes) const;
    int nodeOffset(const ModelNode &node) const;
    int nodeLength(const ModelNode &node) const;
    int firstDefinitionInsideOffset(const ModelNode &node) const;
    int firstDefinitionInsideLength(const ModelNode &node) const;
    bool modificationGroupActive();
    ModelNode nodeAtTextCursorPosition(int cursorPosition) const;

    bool renameId(const QString& oldId, const QString& newId);

    const QmlJS::Document *document() const;
    const QmlJS::ScopeChain *scopeChain() const;

    QString convertTypeToImportAlias(const QString &type) const;

    bool checkSemanticErrors() const
    { return m_checkErrors; }

    void setCheckSemanticErrors(bool b)
    { m_checkErrors = b; }

    QString pathForImport(const Import &import);

    QStringList importDirectories() const;

    QSet<QPair<QString, QString> > qrcMapping() const;

    void moveToComponent(const ModelNode &modelNode);

    QStringList autoComplete(const QString &text, int pos, bool explicitComplete = true);

    QList<QmlTypeData> getQMLTypes() const;

    void setWidgetStatusCallback(std::function<void(bool)> setWidgetStatusCallback);

    void qmlTextChanged();
    void delayedSetup();

    void writeAuxiliaryData();
    void restoreAuxiliaryData();

    QString getRawAuxiliaryData() const;
    QString auxiliaryDataAsQML() const;

    ModelNode getNodeForCanonicalIndex(int index);

signals:
    void modelInterfaceProjectUpdated();

protected: // functions
    void importAdded(const Import &import);
    void importRemoved(const Import &import);

    Internal::ModelToTextMerger *modelToTextMerger() const;
    Internal::TextToModelMerger *textToModelMerger() const;
    bool isModificationGroupActive() const;
    void setModificationGroupActive(bool active);
    void applyModificationGroupChanges();
    void applyChanges();
    void amendQmlText();
    void notifyErrorsAndWarnings(const QList<DocumentMessage> &errors);

private: //variables
    ModelNode nodeAtTextCursorPositionHelper(const ModelNode &root, int cursorPosition) const;
    void setupCanonicalHashes() const;
    void handleLibraryInfoUpdate();
    void handleProjectUpdate();

    TextModifier *m_textModifier = nullptr;
    int transactionLevel = 0;
    bool m_modificationGroupActive = false;
    bool m_checkErrors = true;

    DifferenceHandling m_differenceHandling;
    QScopedPointer<Internal::ModelNodePositionStorage> m_positionStorage;
    QScopedPointer<Internal::ModelToTextMerger> m_modelToTextMerger;
    QScopedPointer<Internal::TextToModelMerger> m_textToModelMerger;
    QList<DocumentMessage> m_errors;
    QList<DocumentMessage> m_warnings;
    RewriterTransaction m_removeDefaultPropertyTransaction;
    QString m_rewritingErrorMessage;
    QString m_lastCorrectQmlSource;
    QTimer m_amendTimer;
    bool m_instantQmlTextUpdate = false;
    std::function<void(bool)> m_setWidgetStatusCallback;
    bool m_hasIncompleteTypeInformation = false;
    bool m_restoringAuxData = false;
    bool m_modelAttachPending = false;

    mutable QHash<int, ModelNode> m_canonicalIntModelNode;
    mutable QHash<ModelNode, int> m_canonicalModelNodeInt;
};

} //QmlDesigner
