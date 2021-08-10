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

#include <coreplugin/inavigationwidgetfactory.h>

#include <QFuture>
#include <QFutureWatcher>
#include <QFutureSynchronizer>
#include <QList>
#include <QSharedPointer>
#include <QStackedWidget>
#include <QStandardItemModel>
#include <QString>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QLabel;
class QModelIndex;
class QStackedLayout;
class QStandardItem;
QT_END_NAMESPACE

namespace TextEditor { class TextEditorLinkLabel; }

namespace Utils {
class AnnotatedItemDelegate;
class NavigationTreeView;
class ProgressIndicator;
}

namespace CppTools { class CppClass; class CppElement; }

namespace CppEditor {
namespace Internal {

class CppEditorWidget;

class CppTypeHierarchyModel : public QStandardItemModel
{
    Q_OBJECT

public:
    CppTypeHierarchyModel(QObject *parent);

    Qt::DropActions supportedDragActions() const override;
    QStringList mimeTypes() const override;
    QMimeData *mimeData(const QModelIndexList &indexes) const override;
};

class CppTypeHierarchyWidget : public QWidget
{
    Q_OBJECT
public:
    CppTypeHierarchyWidget();

    void perform();

private slots:
    void displayHierarchy();

private:
    typedef QList<CppTools::CppClass> CppTools::CppClass::*HierarchyMember;
    void performFromExpression(const QString &expression, const QString &fileName);
    QStandardItem *buildHierarchy(const CppTools::CppClass &cppClass, QStandardItem *parent,
                                  bool isRoot, HierarchyMember member);
    void showNoTypeHierarchyLabel();
    void showTypeHierarchy();
    void showProgress();
    void hideProgress();
    void clearTypeHierarchy();
    void onItemActivated(const QModelIndex &index);
    void onItemDoubleClicked(const QModelIndex &index);
    void updateSynchronizer();

    CppEditorWidget *m_cppEditor = nullptr;
    Utils::NavigationTreeView *m_treeView = nullptr;
    QWidget *m_hierarchyWidget = nullptr;
    QStackedLayout *m_stackLayout = nullptr;
    QStandardItemModel *m_model = nullptr;
    Utils::AnnotatedItemDelegate *m_delegate = nullptr;
    TextEditor::TextEditorLinkLabel *m_inspectedClass = nullptr;
    QLabel *m_infoLabel = nullptr;
    QFuture<QSharedPointer<CppTools::CppElement>> m_future;
    QFutureWatcher<void> m_futureWatcher;
    QFutureSynchronizer<void> m_synchronizer;
    Utils::ProgressIndicator *m_progressIndicator = nullptr;
    QString m_oldClass;
    bool m_showOldClass = false;
};

class CppTypeHierarchyFactory : public Core::INavigationWidgetFactory
{
    Q_OBJECT

public:
    CppTypeHierarchyFactory();

    Core::NavigationView createWidget()  override;
};

} // namespace Internal
} // namespace CppEditor
