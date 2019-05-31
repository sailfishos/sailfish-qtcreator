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

#include "callgrindcostview.h"

#include "callgrindnamedelegate.h"

#include <valgrind/callgrind/callgrindabstractmodel.h>
#include <valgrind/callgrind/callgrinddatamodel.h>
#include <valgrind/callgrind/callgrindfunction.h>
#include <valgrind/callgrind/callgrindcallmodel.h>

#include <QAbstractProxyModel>
#include <QContextMenuEvent>
#include <QMenu>
#include <QDebug>

using namespace Valgrind::Callgrind;

namespace Valgrind {
namespace Internal {

CostView::CostView(QWidget *parent)
    : Utils::BaseTreeView(parent)
    , m_costDelegate(new CostDelegate(this))
    , m_nameDelegate(new NameDelegate(this))
{
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setUniformRowHeights(true);
    setAutoScroll(false);
    setSortingEnabled(true);
    setRootIsDecorated(false);
}

CostView::~CostView() = default;

void CostView::setModel(QAbstractItemModel *model)
{
    BaseTreeView::setModel(model);

    forever {
        auto proxy = qobject_cast<const QAbstractProxyModel *>(model);
        if (!proxy)
            break;
        model = proxy->sourceModel();
    }

    setItemDelegate(new QStyledItemDelegate(this));

    if (qobject_cast<CallModel *>(model)) {
        setItemDelegateForColumn(CallModel::CalleeColumn, m_nameDelegate);
        setItemDelegateForColumn(CallModel::CallerColumn, m_nameDelegate);
        setItemDelegateForColumn(CallModel::CostColumn, m_costDelegate);
    } else if (qobject_cast<DataModel *>(model)) {
        setItemDelegateForColumn(DataModel::InclusiveCostColumn, m_costDelegate);
        setItemDelegateForColumn(DataModel::NameColumn, m_nameDelegate);
        setItemDelegateForColumn(DataModel::SelfCostColumn, m_costDelegate);
    }

    m_costDelegate->setModel(model);
}

void CostView::setCostFormat(CostDelegate::CostFormat format)
{
    m_costDelegate->setFormat(format);
    viewport()->update();
}

CostDelegate::CostFormat CostView::costFormat() const
{
    return m_costDelegate->format();
}

} // namespace Internal
} // namespace Valgrind
