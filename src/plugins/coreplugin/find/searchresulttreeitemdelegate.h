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

#include <QItemDelegate>

namespace Core {
namespace Internal {

struct LayoutInfo
{
    QRect checkRect;
    QRect pixmapRect;
    QRect textRect;
    QRect lineNumberRect;
    QIcon icon;
    Qt::CheckState checkState;
    QStyleOptionViewItem option;
};

class SearchResultTreeItemDelegate: public QItemDelegate
{
public:
    SearchResultTreeItemDelegate(int tabWidth, QObject *parent = nullptr);
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void setTabWidth(int width);
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
private:
    LayoutInfo getLayoutInfo(const QStyleOptionViewItem &option, const QModelIndex &index) const;
    int drawLineNumber(QPainter *painter, const QStyleOptionViewItem &option, const QRect &rect, const QModelIndex &index) const;
    void drawText(QPainter *painter, const QStyleOptionViewItem &option,
                           const QRect &rect, const QModelIndex &index) const;

    QString m_tabString;
};

} // namespace Internal
} // namespace Core
