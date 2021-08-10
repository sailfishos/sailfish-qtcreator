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

#include "cpptypehierarchy.h"

#include "cppeditorconstants.h"
#include "cppeditor.h"
#include "cppeditorwidget.h"
#include "cppeditorplugin.h"

#include <coreplugin/find/itemviewfind.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <cpptools/cppelementevaluator.h>
#include <utils/algorithm.h>
#include <utils/delegates.h>
#include <utils/dropsupport.h>
#include <utils/navigationtreeview.h>
#include <utils/progressindicator.h>

#include <QApplication>
#include <QLabel>
#include <QLatin1String>
#include <QMenu>
#include <QModelIndex>
#include <QStackedLayout>
#include <QVBoxLayout>

using namespace CppEditor;
using namespace CppTools;
using namespace CppEditor::Internal;
using namespace Utils;

namespace {

enum ItemRole {
    AnnotationRole = Qt::UserRole + 1,
    LinkRole
};

QStandardItem *itemForClass(const CppClass &cppClass)
{
    auto item = new QStandardItem;
    item->setFlags(item->flags() | Qt::ItemIsDragEnabled);
    item->setData(cppClass.name, Qt::DisplayRole);
    if (cppClass.name != cppClass.qualifiedName)
        item->setData(cppClass.qualifiedName, AnnotationRole);
    item->setData(cppClass.icon, Qt::DecorationRole);
    QVariant link;
    link.setValue(Link(cppClass.link));
    item->setData(link, LinkRole);
    return item;
}

QList<CppClass> sortClasses(const QList<CppClass> &cppClasses)
{
    QList<CppClass> sorted = cppClasses;
    sort(sorted, [](const CppClass &c1, const CppClass &c2) -> bool {
        const QString key1 = c1.name + QLatin1String("::") + c1.qualifiedName;
        const QString key2 = c2.name + QLatin1String("::") + c2.qualifiedName;
        return key1 < key2;
    });
    return sorted;
}

} // Anonymous

class CppTypeHierarchyTreeView : public NavigationTreeView
{
    Q_OBJECT
public:
    CppTypeHierarchyTreeView(QWidget *parent);

    void contextMenuEvent(QContextMenuEvent *event) override;
};


CppTypeHierarchyTreeView::CppTypeHierarchyTreeView(QWidget *parent) :
    NavigationTreeView(parent)
{
}

void CppTypeHierarchyTreeView::contextMenuEvent(QContextMenuEvent *event)
{
    if (!event)
        return;

    QMenu contextMenu;

    QAction *action = contextMenu.addAction(tr("Open in Editor"));
    connect(action, &QAction::triggered, this, [this] () {
        emit activated(currentIndex());
    });
    action = contextMenu.addAction(tr("Open Type Hierarchy"));
    connect(action, &QAction::triggered, this, [this] () {
        emit doubleClicked(currentIndex());
    });

    contextMenu.addSeparator();

    action = contextMenu.addAction(tr("Expand All"));
    connect(action, &QAction::triggered, this, &QTreeView::expandAll);
    action = contextMenu.addAction(tr("Collapse All"));
    connect(action, &QAction::triggered, this, &QTreeView::collapseAll);

    contextMenu.exec(event->globalPos());

    event->accept();
}

namespace CppEditor {
namespace Internal {

// CppTypeHierarchyWidget
CppTypeHierarchyWidget::CppTypeHierarchyWidget()
{
    m_inspectedClass = new TextEditor::TextEditorLinkLabel(this);
    m_inspectedClass->setContentsMargins(5, 5, 5, 5);
    m_model = new CppTypeHierarchyModel(this);
    m_treeView = new CppTypeHierarchyTreeView(this);
    m_treeView->setActivationMode(SingleClickActivation);
    m_delegate = new AnnotatedItemDelegate(this);
    m_delegate->setDelimiter(QLatin1String(" "));
    m_delegate->setAnnotationRole(AnnotationRole);
    m_treeView->setModel(m_model);
    m_treeView->setExpandsOnDoubleClick(false);
    m_treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_treeView->setItemDelegate(m_delegate);
    m_treeView->setRootIsDecorated(false);
    m_treeView->setDragEnabled(true);
    m_treeView->setDragDropMode(QAbstractItemView::DragOnly);
    m_treeView->setDefaultDropAction(Qt::MoveAction);
    connect(m_treeView, &QTreeView::activated, this, &CppTypeHierarchyWidget::onItemActivated);
    connect(m_treeView, &QTreeView::doubleClicked, this, &CppTypeHierarchyWidget::onItemDoubleClicked);

    m_infoLabel = new QLabel(this);
    m_infoLabel->setAlignment(Qt::AlignCenter);
    m_infoLabel->setAutoFillBackground(true);
    m_infoLabel->setBackgroundRole(QPalette::Base);

    m_hierarchyWidget = new QWidget(this);
    auto layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_inspectedClass);
    layout->addWidget(Core::ItemViewFind::createSearchableWrapper(m_treeView));
    m_hierarchyWidget->setLayout(layout);

    m_stackLayout = new QStackedLayout;
    m_stackLayout->addWidget(m_hierarchyWidget);
    m_stackLayout->addWidget(m_infoLabel);
    showNoTypeHierarchyLabel();
    setLayout(m_stackLayout);

    connect(CppEditorPlugin::instance(), &CppEditorPlugin::typeHierarchyRequested,
            this, &CppTypeHierarchyWidget::perform);
    connect(&m_futureWatcher, &QFutureWatcher<void>::finished,
            this, &CppTypeHierarchyWidget::displayHierarchy);

    m_synchronizer.setCancelOnWait(true);
}

void CppTypeHierarchyWidget::updateSynchronizer()
{
    const QList<QFuture<void>> futures = m_synchronizer.futures();

    m_synchronizer.clearFutures();

    for (const QFuture<void> &future : futures) {
        if (!future.isFinished())
            m_synchronizer.addFuture(future);
    }
}

void CppTypeHierarchyWidget::perform()
{
    if (m_future.isRunning())
        m_future.cancel();

    m_showOldClass = false;

    updateSynchronizer();

    auto editor = qobject_cast<CppEditor *>(Core::EditorManager::currentEditor());
    if (!editor) {
        showNoTypeHierarchyLabel();
        return;
    }

    auto widget = qobject_cast<CppEditorWidget *>(editor->widget());
    if (!widget) {
        showNoTypeHierarchyLabel();
        return;
    }

    showProgress();

    m_future = CppElementEvaluator::asyncExecute(widget);
    m_futureWatcher.setFuture(QFuture<void>(m_future));
    m_synchronizer.addFuture(QFuture<void>(m_future));

    Core::ProgressManager::addTask(m_future, tr("Evaluating Type Hierarchy"), "TypeHierarchy");
}

void CppTypeHierarchyWidget::performFromExpression(const QString &expression, const QString &fileName)
{
    if (m_future.isRunning())
        m_future.cancel();

    m_showOldClass = true;

    updateSynchronizer();

    showProgress();

    m_future = CppElementEvaluator::asyncExecute(expression, fileName);
    m_futureWatcher.setFuture(QFuture<void>(m_future));
    m_synchronizer.addFuture(QFuture<void>(m_future));

    Core::ProgressManager::addTask(m_future, tr("Evaluating Type Hierarchy"), "TypeHierarchy");
}

void CppTypeHierarchyWidget::displayHierarchy()
{
    updateSynchronizer();
    hideProgress();
    clearTypeHierarchy();

    if (!m_future.resultCount() || m_future.isCanceled()) {
        showNoTypeHierarchyLabel();
        return;
    }
    const QSharedPointer<CppElement> &cppElement = m_future.result();
    if (cppElement.isNull()) {
        showNoTypeHierarchyLabel();
        return;
    }
    CppClass *cppClass = cppElement->toCppClass();
    if (!cppClass) {
        showNoTypeHierarchyLabel();
        return;
    }

    m_inspectedClass->setText(cppClass->name);
    m_inspectedClass->setLink(cppClass->link);
    QStandardItem *bases = new QStandardItem(tr("Bases"));
    m_model->invisibleRootItem()->appendRow(bases);
    QStandardItem *selectedItem1 = buildHierarchy(*cppClass, bases, true, &CppClass::bases);
    QStandardItem *derived = new QStandardItem(tr("Derived"));
    m_model->invisibleRootItem()->appendRow(derived);
    QStandardItem *selectedItem2 = buildHierarchy(*cppClass, derived, true, &CppClass::derived);
    m_treeView->expandAll();
    m_oldClass = cppClass->qualifiedName;

    QStandardItem *selectedItem = selectedItem1 ? selectedItem1 : selectedItem2;
    if (selectedItem)
        m_treeView->setCurrentIndex(m_model->indexFromItem(selectedItem));

    showTypeHierarchy();
}

QStandardItem *CppTypeHierarchyWidget::buildHierarchy(const CppClass &cppClass, QStandardItem *parent,
                                            bool isRoot, const HierarchyMember member)
{
    QStandardItem *selectedItem = nullptr;
    if (!isRoot) {
        QStandardItem *item = itemForClass(cppClass);
        parent->appendRow(item);
        parent = item;
        if (m_showOldClass && cppClass.qualifiedName == m_oldClass)
            selectedItem = item;
    }
    foreach (const CppClass &klass, sortClasses(cppClass.*member)) {
        QStandardItem *item = buildHierarchy(klass, parent, false, member);
        if (!selectedItem)
            selectedItem = item;
    }
    return selectedItem;
}

void CppTypeHierarchyWidget::showNoTypeHierarchyLabel()
{
    m_infoLabel->setText(tr("No type hierarchy available"));
    m_stackLayout->setCurrentWidget(m_infoLabel);
}

void CppTypeHierarchyWidget::showTypeHierarchy()
{
    m_stackLayout->setCurrentWidget(m_hierarchyWidget);
}

void CppTypeHierarchyWidget::showProgress()
{
    m_infoLabel->setText(tr("Evaluating type hierarchy..."));
    if (!m_progressIndicator) {
        m_progressIndicator = new ProgressIndicator(ProgressIndicatorSize::Large);
        m_progressIndicator->attachToWidget(this);
    }
    m_progressIndicator->show();
    m_progressIndicator->raise();
}
void CppTypeHierarchyWidget::hideProgress()
{
    if (m_progressIndicator)
        m_progressIndicator->hide();
}

void CppTypeHierarchyWidget::clearTypeHierarchy()
{
    m_inspectedClass->clear();
    m_model->clear();
}

static QString getExpression(const QModelIndex &index)
{
    const QString annotation = index.data(AnnotationRole).toString();
    if (!annotation.isEmpty())
        return annotation;
    return index.data(Qt::DisplayRole).toString();
}

void CppTypeHierarchyWidget::onItemActivated(const QModelIndex &index)
{
    auto link = index.data(LinkRole).value<Link>();
    if (!link.hasValidTarget())
        return;

    const Link updatedLink = CppElementEvaluator::linkFromExpression(
                getExpression(index), link.targetFileName);
    if (updatedLink.hasValidTarget())
        link = updatedLink;

    Core::EditorManager::openEditorAt(link.targetFileName,
                                      link.targetLine,
                                      link.targetColumn,
                                      Constants::CPPEDITOR_ID);
}

void CppTypeHierarchyWidget::onItemDoubleClicked(const QModelIndex &index)
{
    const auto link = index.data(LinkRole).value<Link>();
    if (link.hasValidTarget())
        performFromExpression(getExpression(index), link.targetFileName);
}

// CppTypeHierarchyFactory
CppTypeHierarchyFactory::CppTypeHierarchyFactory()
{
    setDisplayName(tr("Type Hierarchy"));
    setPriority(700);
    setId(Constants::TYPE_HIERARCHY_ID);
}

Core::NavigationView CppTypeHierarchyFactory::createWidget()
{
    auto w = new CppTypeHierarchyWidget;
    w->perform();
    return Core::NavigationView(w);
}

CppTypeHierarchyModel::CppTypeHierarchyModel(QObject *parent)
    : QStandardItemModel(parent)
{
}

Qt::DropActions CppTypeHierarchyModel::supportedDragActions() const
{
    // copy & move actions to avoid idiotic behavior of drag and drop:
    // standard item model removes nodes automatically that are
    // dropped anywhere with move action, but we do not want the '+' sign in the
    // drag handle that would appear when only allowing copy action
    return Qt::CopyAction | Qt::MoveAction;
}

QStringList CppTypeHierarchyModel::mimeTypes() const
{
    return DropSupport::mimeTypesForFilePaths();
}

QMimeData *CppTypeHierarchyModel::mimeData(const QModelIndexList &indexes) const
{
    auto data = new DropMimeData;
    data->setOverrideFileDropAction(Qt::CopyAction); // do not remove the item from the model
    foreach (const QModelIndex &index, indexes) {
        auto link = index.data(LinkRole).value<Link>();
        if (link.hasValidTarget())
            data->addFile(link.targetFileName, link.targetLine, link.targetColumn);
    }
    return data;
}

} // namespace Internal
} // namespace CppEditor

#include "cpptypehierarchy.moc"
