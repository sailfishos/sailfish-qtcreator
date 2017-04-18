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

#include <QTreeView>
#include <QStyledItemDelegate>

QT_BEGIN_NAMESPACE
class QFileSystemModel;
class QLabel;
QT_END_NAMESPACE

namespace QmlDesigner {

class ResourceItemDelegate;

// ItemLibraryTreeView with Drag implementation
class ItemLibraryTreeView : public QTreeView {

    Q_OBJECT
public:
    explicit ItemLibraryTreeView(QWidget *parent = 0);

    virtual void startDrag(Qt::DropActions supportedActions);
    virtual void setModel(QAbstractItemModel *model);

    static void drawSelectionBackground(QPainter *painter, const QStyleOption &option);

signals:
    void itemActivated(const QString &itemName);

private slots:
    void activateItem( const QModelIndex &index);

private:
    ResourceItemDelegate *m_delegate;
};

} // namespace QmlDesigner
