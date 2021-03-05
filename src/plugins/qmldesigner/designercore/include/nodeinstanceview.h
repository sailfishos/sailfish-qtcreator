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
#include "abstractview.h"

#include <modelnode.h>
#include <nodeinstance.h>
#include <nodeinstanceclientinterface.h>
#include <nodeinstanceserverinterface.h>

#include <QElapsedTimer>
#include <QHash>
#include <QImage>
#include <QPointer>
#include <QRectF>
#include <QTime>
#include <QTimer>
#include <QtGui/qevent.h>

#include <memory>

QT_FORWARD_DECLARE_CLASS(QFileSystemWatcher)

namespace ProjectExplorer {
class Target;
}

namespace QmlDesigner {

class NodeInstanceServerProxy;
class CreateSceneCommand;
class CreateInstancesCommand;
class ClearSceneCommand;
class ReparentInstancesCommand;
class ChangeFileUrlCommand;
class ChangeValuesCommand;
class ChangeBindingsCommand;
class ChangeIdsCommand;
class RemoveInstancesCommand;
class ChangeSelectionCommand;
class RemovePropertiesCommand;
class CompleteComponentCommand;
class InformationContainer;
class TokenCommand;
class ConnectionManagerInterface;

class QMLDESIGNERCORE_EXPORT NodeInstanceView : public AbstractView, public NodeInstanceClientInterface
{
    Q_OBJECT

    friend class NodeInstance;

public:
    using Pointer = QWeakPointer<NodeInstanceView>;

    explicit NodeInstanceView(ConnectionManagerInterface &connectionManager);
    ~NodeInstanceView() override;

    void modelAttached(Model *model) override;
    void modelAboutToBeDetached(Model *model) override;
    void nodeCreated(const ModelNode &createdNode) override;
    void nodeAboutToBeRemoved(const ModelNode &removedNode) override;
    void propertiesAboutToBeRemoved(const QList<AbstractProperty>& propertyList) override;
    void variantPropertiesChanged(const QList<VariantProperty>& propertyList, PropertyChangeFlags propertyChange) override;
    void bindingPropertiesChanged(const QList<BindingProperty>& propertyList, PropertyChangeFlags propertyChange) override;
    void nodeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent,
                        const NodeAbstractProperty &oldPropertyParent,
                        AbstractView::PropertyChangeFlags propertyChange) override;
    void rootNodeTypeChanged(const QString &type, int majorVersion, int minorVersion) override;
    void nodeTypeChanged(const ModelNode& node, const TypeName &type, int majorVersion, int minorVersion) override;
    void fileUrlChanged(const QUrl &oldUrl, const QUrl &newUrl) override;
    void nodeIdChanged(const ModelNode& node, const QString& newId, const QString& oldId) override;
    void nodeOrderChanged(const NodeListProperty &listProperty, const ModelNode &movedNode, int oldIndex) override;
    void importsChanged(const QList<Import> &addedImports, const QList<Import> &removedImports) override;
    void auxiliaryDataChanged(const ModelNode &node, const PropertyName &name, const QVariant &data) override;
    void customNotification(const AbstractView *view, const QString &identifier, const QList<ModelNode> &nodeList, const QList<QVariant> &data) override;
    void nodeSourceChanged(const ModelNode &modelNode, const QString &newNodeSource) override;
    void capturedData(const CapturedDataCommand &capturedData) override;
    void currentStateChanged(const ModelNode &node) override;
    void sceneCreated(const SceneCreatedCommand &command) override;

    QList<NodeInstance> instances() const;
    NodeInstance instanceForModelNode(const ModelNode &node) const ;
    bool hasInstanceForModelNode(const ModelNode &node) const;

    NodeInstance instanceForId(qint32 id);
    bool hasInstanceForId(qint32 id);

    QRectF sceneRect() const;

    NodeInstance activeStateInstance() const;

    void updateChildren(const NodeAbstractProperty &newPropertyParent);
    void updatePosition(const QList<VariantProperty>& propertyList);

    void valuesChanged(const ValuesChangedCommand &command) override;
    void valuesModified(const ValuesModifiedCommand &command) override;
    void pixmapChanged(const PixmapChangedCommand &command) override;
    void informationChanged(const InformationChangedCommand &command) override;
    void childrenChanged(const ChildrenChangedCommand &command) override;
    void statePreviewImagesChanged(const StatePreviewImageChangedCommand &command) override;
    void componentCompleted(const ComponentCompletedCommand &command) override;
    void token(const TokenCommand &command) override;
    void debugOutput(const DebugOutputCommand &command) override;

    QImage statePreviewImage(const ModelNode &stateNode) const;

    void setTarget(ProjectExplorer::Target *newTarget);

    void sendToken(const QString &token, int number, const QVector<ModelNode> &nodeVector);

    void selectionChanged(const ChangeSelectionCommand &command) override;

    void selectedNodesChanged(const QList<ModelNode> &selectedNodeList,
                              const QList<ModelNode> &lastSelectedNodeList) override;

    void sendInputEvent(QInputEvent *e) const;
    void view3DAction(const View3DActionCommand &command);
    void requestModelNodePreviewImage(const ModelNode &node, const ModelNode &renderNode);
    void edit3DViewResized(const QSize &size) const;

    void handlePuppetToCreatorCommand(const PuppetToCreatorCommand &command) override;

    QVariant previewImageDataForGenericNode(const ModelNode &modelNode, const ModelNode &renderNode);
    QVariant previewImageDataForImageNode(const ModelNode &modelNode);

    void setCrashCallback(std::function<void()> crashCallback)
    {
        m_crashCallback = std::move(crashCallback);
    }

protected:
    void timerEvent(QTimerEvent *event) override;

private: // functions
    std::unique_ptr<NodeInstanceServerProxy> createNodeInstanceServerProxy();
    void activateState(const NodeInstance &instance);
    void activateBaseState();

    NodeInstance rootNodeInstance() const;

    NodeInstance loadNode(const ModelNode &node);

    void removeAllInstanceNodeRelationships();

    void removeRecursiveChildRelationship(const ModelNode &removedNode);

    void insertInstanceRelationships(const NodeInstance &instance);
    void removeInstanceNodeRelationship(const ModelNode &node);

    void removeInstanceAndSubInstances(const ModelNode &node);

    void setStateInstance(const NodeInstance &stateInstance);
    void clearStateInstance();

    QMultiHash<ModelNode, InformationName> informationChanged(
        const QVector<InformationContainer> &containerVector);

    CreateSceneCommand createCreateSceneCommand();
    ClearSceneCommand createClearSceneCommand() const;
    CreateInstancesCommand createCreateInstancesCommand(const QList<NodeInstance> &instanceList) const;
    CompleteComponentCommand createComponentCompleteCommand(const QList<NodeInstance> &instanceList) const;
    ComponentCompletedCommand createComponentCompletedCommand(const QList<NodeInstance> &instanceList) const;
    ReparentInstancesCommand createReparentInstancesCommand(const QList<NodeInstance> &instanceList) const;
    ReparentInstancesCommand createReparentInstancesCommand(const ModelNode &node, const NodeAbstractProperty &newPropertyParent, const NodeAbstractProperty &oldPropertyParent) const;
    ChangeFileUrlCommand createChangeFileUrlCommand(const QUrl &fileUrl) const;
    ChangeValuesCommand createChangeValueCommand(const QList<VariantProperty> &propertyList) const;
    ChangeBindingsCommand createChangeBindingCommand(const QList<BindingProperty> &propertyList) const;
    ChangeIdsCommand createChangeIdsCommand(const QList<NodeInstance> &instanceList) const;
    RemoveInstancesCommand createRemoveInstancesCommand(const QList<ModelNode> &nodeList) const;
    ChangeSelectionCommand createChangeSelectionCommand(const QList<ModelNode> &nodeList) const;
    RemoveInstancesCommand createRemoveInstancesCommand(const ModelNode &node) const;
    RemovePropertiesCommand createRemovePropertiesCommand(const QList<AbstractProperty> &propertyList) const;
    RemoveSharedMemoryCommand createRemoveSharedMemoryCommand(const QString &sharedMemoryTypeName, quint32 keyNumber);
    RemoveSharedMemoryCommand createRemoveSharedMemoryCommand(const QString &sharedMemoryTypeName, const QList<ModelNode> &nodeList);

    void resetHorizontalAnchors(const ModelNode &node);
    void resetVerticalAnchors(const ModelNode &node);

    void restartProcess();
    void delayedRestartProcess();

    void handleCrash();
    void startPuppetTransaction();
    void endPuppetTransaction();

    // puppet to creator command handlers
    void handlePuppetKeyPress(int key, Qt::KeyboardModifiers modifiers);

    struct ModelNodePreviewImageData {
        QDateTime time;
        QPixmap pixmap;
        QString type;
        QString id;
        QString info;
    };
    QVariant modelNodePreviewImageDataToVariant(const ModelNodePreviewImageData &imageData);
    void updatePreviewImageForNode(const ModelNode &modelNode, const QImage &image);

    void updateWatcher(const QString &path);

private:
    QHash<QString, ModelNodePreviewImageData> m_imageDataMap;

    NodeInstance m_rootNodeInstance;
    NodeInstance m_activeStateInstance;
    QHash<ModelNode, NodeInstance> m_nodeInstanceHash;
    QHash<ModelNode, QImage> m_statePreviewImage;
    ConnectionManagerInterface &m_connectionManager;
    std::unique_ptr<NodeInstanceServerProxy> m_nodeInstanceServer;
    QImage m_baseStatePreviewImage;
    QElapsedTimer m_lastCrashTime;
    ProjectExplorer::Target *m_currentTarget = nullptr;
    int m_restartProcessTimerId;
    RewriterTransaction m_puppetTransaction;

    // key: fileUrl value: (key: instance qml id, value: related tool states)
    QHash<QUrl, QHash<QString, QVariantMap>> m_edit3DToolStates;

    std::function<void()> m_crashCallback{[this] { handleCrash(); }};

    // We use QFileSystemWatcher directly instead of Utils::FileSystemWatcher as we want
    // shader changes to be applied immediately rather than requiring reactivation of
    // the creator application.
    QFileSystemWatcher *m_fileSystemWatcher;
    QTimer m_resetTimer;
    QTimer m_updateWatcherTimer;
    QSet<QString> m_pendingUpdateDirs;
};

} // namespace ProxyNodeInstanceView
