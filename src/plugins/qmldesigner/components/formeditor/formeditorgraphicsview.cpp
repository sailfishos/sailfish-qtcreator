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

#include "formeditorgraphicsview.h"
#include "formeditoritem.h"
#include "formeditorwidget.h"
#include "navigation2d.h"

#include <QAction>
#include <QCoreApplication>
#include <QGraphicsItem>
#include <QGraphicsProxyWidget>
#include <QGraphicsWidget>
#include <QScrollBar>
#include <QWheelEvent>

#include <QTimer>

namespace QmlDesigner {

FormEditorGraphicsView::FormEditorGraphicsView(QWidget *parent)
    : QGraphicsView(parent)
{
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);
    setAlignment(Qt::AlignCenter);
    setCacheMode(QGraphicsView::CacheNone);
    setViewportUpdateMode(QGraphicsView::MinimalViewportUpdate);
    setOptimizationFlags(QGraphicsView::DontSavePainterState);
    setRenderHint(QPainter::Antialiasing, false);

    setFrameShape(QFrame::NoFrame);

    setAutoFillBackground(true);
    setBackgroundRole(QPalette::Window);

    activateCheckboardBackground();

    // as mousetracking only works for mouse key it is better to handle it in the
    // eventFilter method so it works also for the space scrolling case as expected
    QCoreApplication::instance()->installEventFilter(this);

    QmlDesigner::Navigation2dFilter *filter = new QmlDesigner::Navigation2dFilter(this);
    connect(filter, &Navigation2dFilter::zoomIn, this, &FormEditorGraphicsView::zoomIn);
    connect(filter, &Navigation2dFilter::zoomOut, this, &FormEditorGraphicsView::zoomOut);

    auto panChanged = &Navigation2dFilter::panChanged;
    connect(filter, panChanged, [this](const QPointF &direction) {
        QScrollBar *sbx = horizontalScrollBar();
        QScrollBar *sby = verticalScrollBar();

        // max - min + pageStep = sceneRect.size * scale
        QPointF min(sbx->minimum(), sby->minimum());
        QPointF max(sbx->maximum(), sby->maximum());
        QPointF step(sbx->pageStep(), sby->pageStep());

        QPointF d1 = max - min;
        QPointF d2 = d1 + step;

        QPoint val = QPointF((direction.x() / d2.x()) * d1.x(), (direction.y() / d2.y()) * d1.y())
                         .toPoint();

        sbx->setValue(sbx->value() - val.x());
        sby->setValue(sby->value() - val.y());
    });

    auto zoomChanged = &Navigation2dFilter::zoomChanged;
    connect(filter, zoomChanged, [this](double s, const QPointF &/*pos*/) {
        if (auto trans = transform() * QTransform::fromScale(1.0 + s, 1.0 + s); trans.m11() > 0) {
            setTransform(trans);
            emit this->zoomChanged(transform().m11());
        }
    });
    installEventFilter(filter);
}

bool FormEditorGraphicsView::eventFilter(QObject *watched, QEvent *event)
{
    if (m_isPanning != Panning::NotStarted) {
        if (event->type() == QEvent::Leave && m_isPanning == Panning::SpaceKeyStarted) {
            // there is no way to keep the cursor so we stop panning here
            stopPanning(event);
        }
        if (event->type() == QEvent::MouseMove) {
            auto mouseEvent = static_cast<QMouseEvent *>(event);
            if (!m_panningStartPosition.isNull()) {
                horizontalScrollBar()->setValue(horizontalScrollBar()->value() -
                    (mouseEvent->x() - m_panningStartPosition.x()));
                verticalScrollBar()->setValue(verticalScrollBar()->value() -
                    (mouseEvent->y() - m_panningStartPosition.y()));
            }
            m_panningStartPosition = mouseEvent->pos();
            event->accept();
            return true;
        }
    }
    return QGraphicsView::eventFilter(watched, event);
}

void FormEditorGraphicsView::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers().testFlag(Qt::ControlModifier))
        event->ignore();
    else if (event->source() == Qt::MouseEventNotSynthesized)
        QGraphicsView::wheelEvent(event);
}

void FormEditorGraphicsView::mousePressEvent(QMouseEvent *event)
{
    if (m_isPanning == Panning::NotStarted) {
        if (event->buttons().testFlag(Qt::MiddleButton))
            startPanning(event);
        else
            QGraphicsView::mousePressEvent(event);
    }
}

void FormEditorGraphicsView::mouseReleaseEvent(QMouseEvent *event)
{
    // not sure why buttons() are empty here, but we have that information from the enum
    if (m_isPanning == Panning::MouseWheelStarted)
        stopPanning(event);
    else
        QGraphicsView::mouseReleaseEvent(event);
}

bool isTextInputItem(QGraphicsItem *item)
{
    if (item && item->isWidget()) {
        auto graphicsWidget = static_cast<QGraphicsWidget *>(item);
        auto textInputProxyWidget = qobject_cast<QGraphicsProxyWidget *>(graphicsWidget);
        if (textInputProxyWidget && textInputProxyWidget->widget() && (
                strcmp(textInputProxyWidget->widget()->metaObject()->className(), "QLineEdit") == 0 ||
                strcmp(textInputProxyWidget->widget()->metaObject()->className(), "QTextEdit") == 0)) {
            return true;
        }

    }
    return false;
}

void FormEditorGraphicsView::keyPressEvent(QKeyEvent *event)
{
    // check for autorepeat to avoid a stoped space panning by leave event to be restarted
    if (!event->isAutoRepeat() && m_isPanning == Panning::NotStarted && event->key() == Qt::Key_Space &&
        !isTextInputItem(scene()->focusItem())) {
        startPanning(event);
        return;
    }
    QGraphicsView::keyPressEvent(event);
}

void FormEditorGraphicsView::keyReleaseEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Space && !event->isAutoRepeat() && m_isPanning == Panning::SpaceKeyStarted)
        stopPanning(event);

    QGraphicsView::keyReleaseEvent(event);
}

void FormEditorGraphicsView::startPanning(QEvent *event)
{
    if (event->type() == QEvent::KeyPress)
        m_isPanning = Panning::SpaceKeyStarted;
    else
        m_isPanning = Panning::MouseWheelStarted;
    viewport()->setCursor(Qt::ClosedHandCursor);
    event->accept();
}

void FormEditorGraphicsView::stopPanning(QEvent *event)
{
    m_isPanning = Panning::NotStarted;
    m_panningStartPosition = QPoint();
    viewport()->unsetCursor();
    event->accept();
}

void FormEditorGraphicsView::setRootItemRect(const QRectF &rect)
{
    m_rootItemRect = rect;
    viewport()->update();
}

QRectF FormEditorGraphicsView::rootItemRect() const
{
    return m_rootItemRect;
}

void FormEditorGraphicsView::activateCheckboardBackground()
{
    const int checkerbordSize = 20;
    QPixmap tilePixmap(checkerbordSize * 2, checkerbordSize * 2);
    tilePixmap.fill(Qt::white);
    QPainter tilePainter(&tilePixmap);
    QColor color(220, 220, 220);
    tilePainter.fillRect(0, 0, checkerbordSize, checkerbordSize, color);
    tilePainter.fillRect(checkerbordSize, checkerbordSize, checkerbordSize, checkerbordSize, color);
    tilePainter.end();

    setBackgroundBrush(tilePixmap);
}

void FormEditorGraphicsView::activateColoredBackground(const QColor &color)
{
    setBackgroundBrush(color);
}

void FormEditorGraphicsView::drawBackground(QPainter *painter, const QRectF &rectangle)
{
    painter->save();
    painter->setBrushOrigin(0, 0);

    painter->fillRect(rectangle.intersected(rootItemRect()), backgroundBrush());
    // paint rect around editable area
    painter->setPen(Qt::black);
    painter->drawRect(rootItemRect());
    painter->restore();
}

void FormEditorGraphicsView::frame(const QRectF &boundingRect)
{
    fitInView(boundingRect, Qt::KeepAspectRatio);
}

void FormEditorGraphicsView::setZoomFactor(double zoom)
{
    resetTransform();
    scale(zoom, zoom);
}

} // namespace QmlDesigner
