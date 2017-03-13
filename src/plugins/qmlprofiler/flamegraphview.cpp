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

#include "flamegraphview.h"
#include "flamegraph.h"
#include "qmlprofilerconstants.h"
#include "qmlprofilertool.h"

#include <QQmlContext>
#include <QVBoxLayout>
#include <QMenu>

namespace QmlProfiler {
namespace Internal {

FlameGraphView::FlameGraphView(QmlProfilerModelManager *manager, QWidget *parent) :
    QmlProfilerEventsView(parent), m_content(new QQuickWidget(this)),
    m_model(new FlameGraphModel(manager, this))
{
    setWindowTitle(QStringLiteral("Flamegraph"));
    setObjectName(QStringLiteral("QmlProfilerFlamegraph"));

    qmlRegisterType<FlameGraph>("FlameGraph", 1, 0, "FlameGraph");
    qmlRegisterUncreatableType<FlameGraphModel>("FlameGraphModel", 1, 0, "FlameGraphModel",
                                                QLatin1String("use the context property"));
    qmlRegisterUncreatableType<QAbstractItemModel>("AbstractItemModel", 1, 0, "AbstractItemModel",
                                                   QLatin1String("only for Qt 5.4"));

    m_content->rootContext()->setContextProperty(QStringLiteral("flameGraphModel"), m_model);
    m_content->setSource(QUrl(QStringLiteral("qrc:/qmlprofiler/FlameGraphView.qml")));
    m_content->setClearColor(QColor(0xdc, 0xdc, 0xdc));

    m_content->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_content->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_content);
    setLayout(layout);

    connect(m_content->rootObject(), SIGNAL(typeSelected(int)),
            this, SIGNAL(typeSelected(int)));
    connect(m_content->rootObject(), SIGNAL(gotoSourceLocation(QString,int,int)),
            this, SIGNAL(gotoSourceLocation(QString,int,int)));
}

void FlameGraphView::selectByTypeId(int typeIndex)
{
    m_content->rootObject()->setProperty("selectedTypeId", typeIndex);
}

void FlameGraphView::onVisibleFeaturesChanged(quint64 features)
{
    int rangeTypeMask = 0;
    for (int rangeType = 0; rangeType < MaximumRangeType; ++rangeType) {
        if (features & (1ULL << featureFromRangeType(RangeType(rangeType))))
            rangeTypeMask |= (1 << rangeType);
    }
    m_content->rootObject()->setProperty("visibleRangeTypes", rangeTypeMask);
}

void FlameGraphView::contextMenuEvent(QContextMenuEvent *ev)
{
    QMenu menu;
    QAction *getGlobalStatsAction = 0;

    QPoint position = ev->globalPos();

    menu.addActions(QmlProfilerTool::profilerContextMenuActions());
    menu.addSeparator();
    getGlobalStatsAction = menu.addAction(tr("Show Full Range"));
    if (!m_model->modelManager()->isRestrictedToRange())
        getGlobalStatsAction->setEnabled(false);

    if (menu.exec(position) == getGlobalStatsAction)
        emit showFullRange();
}

} // namespace Internal
} // namespace QmlProfiler
