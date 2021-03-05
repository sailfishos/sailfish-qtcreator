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

#include "timelinemovetool.h"

#include "timelinegraphicsscene.h"
#include "timelinemovableabstractitem.h"
#include "timelinepropertyitem.h"
#include "timelineview.h"

#include <exception.h>

#include <QApplication>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>

#include <cmath>

namespace QmlDesigner {

static QPointF mapPointToItem(TimelineMovableAbstractItem *item, const QPointF &pos)
{
    if (auto parent = item->parentItem())
        return parent->mapFromScene(pos);
    return pos;
}

QPointF mapToItem(TimelineMovableAbstractItem *item, const QPointF &pos)
{
    if (auto parent = item->parentItem())
        return parent->mapFromScene(pos);
    return pos;
}

QPointF mapToItem(TimelineMovableAbstractItem *item, QGraphicsSceneMouseEvent *event)
{
    if (auto parent = item->parentItem())
        return parent->mapFromScene(event->scenePos());
    return event->scenePos();
}

TimelineMoveTool::TimelineMoveTool(AbstractScrollGraphicsScene *scene, TimelineToolDelegate *delegate)
    : TimelineAbstractTool(scene, delegate)
{}

void TimelineMoveTool::mousePressEvent(TimelineMovableAbstractItem *item,
                                       QGraphicsSceneMouseEvent *event)
{
    Q_UNUSED(item)

    if (currentItem() && currentItem()->isLocked())
        return;

    if (auto *current = currentItem()->asTimelineKeyframeItem()) {
        const qreal sourceFrame = qRound(current->mapFromSceneToFrame(current->rect().center().x()));
        const qreal targetFrame = qRound(current->mapFromSceneToFrame(event->scenePos().x()));
        m_pressKeyframeDelta = targetFrame - sourceFrame;
    }
}

void TimelineMoveTool::mouseMoveEvent(TimelineMovableAbstractItem *item,
                                      QGraphicsSceneMouseEvent *event)
{
    Q_UNUSED(item)

    if (!currentItem())
        return;

    if (currentItem()->isLocked())
        return;

    if (auto *current = currentItem()->asTimelineKeyframeItem()) {
        // prevent dragging if deselecting a keyframe (Ctrl+click and drag a selected keyframe)
        if (!current->highlighted())
            return;

        const qreal sourceFrame = qRound(current->mapFromSceneToFrame(current->rect().center().x()));
        qreal targetFrame = qRound(current->mapFromSceneToFrame(event->scenePos().x()));
        qreal deltaFrame = targetFrame - sourceFrame - m_pressKeyframeDelta;

        const qreal minFrame = scene()->startFrame();
        const qreal maxFrame = scene()->endFrame();

        auto bbox = scene()->selectionBounds().adjusted(TimelineConstants::keyFrameSize / 2, 0,
                                                        -TimelineConstants::keyFrameSize / 2, 0);
        double firstFrame = std::round(current->mapFromSceneToFrame(bbox.left()));
        double lastFrame = std::round(current->mapFromSceneToFrame(bbox.right()));

        if (lastFrame + deltaFrame > maxFrame)
            deltaFrame = maxFrame - lastFrame;
        else if (firstFrame + deltaFrame < minFrame)
            deltaFrame = minFrame - firstFrame;

        targetFrame = sourceFrame + deltaFrame;

        if (QApplication::keyboardModifiers() & Qt::ShiftModifier) { // keyframe snapping
            qreal snappedTargetFrame = scene()->snap(targetFrame);
            deltaFrame += snappedTargetFrame - targetFrame;
            targetFrame = snappedTargetFrame;
        }

        scene()->statusBarMessageChanged(tr(TimelineConstants::statusBarKeyframe)
                                         .arg(targetFrame));

        const QList<TimelineKeyframeItem *> selectedKeyframes = scene()->selectedKeyframes();
        for (auto *keyframe : selectedKeyframes) {
            qreal pos = std::round(keyframe->mapFromSceneToFrame(keyframe->rect().center().x()));
            keyframe->setPosition(pos + deltaFrame);
        }

    } else {
        currentItem()->itemMoved(mapPointToItem(currentItem(), startPosition()),
                                 mapToItem(currentItem(), event));
    }
}

void TimelineMoveTool::mouseReleaseEvent(TimelineMovableAbstractItem *item,
                                         QGraphicsSceneMouseEvent *event)
{
    Q_UNUSED(item)

    if (auto *current = currentItem()) {
        if (current->asTimelineFrameHandle()) {
            double mousePos = event->scenePos().x();
            double start = current->mapFromFrameToScene(scene()->startFrame());
            double end = current->mapFromFrameToScene(scene()->endFrame());

            double limitFrame = -999999.;
            if (mousePos < start)
                limitFrame = scene()->startFrame();
            else if (mousePos > end)
                limitFrame = scene()->endFrame();

            if (limitFrame > -999999.) {
                scene()->setCurrentFrame(limitFrame);
                scene()->statusBarMessageChanged(
                            tr(TimelineConstants::statusBarPlayheadFrame).arg(limitFrame));
                return;
            }
        }

        scene()->abstractView()->executeInTransaction("TimelineMoveTool::mouseReleaseEvent",
                                                      [this, current]() {
            current->commitPosition(mapToItem(current, current->rect().center()));

            if (current->asTimelineKeyframeItem()) {
                double frame = std::round(
                            current->mapFromSceneToFrame(current->rect().center().x()));

                scene()->statusBarMessageChanged(
                            tr(TimelineConstants::statusBarKeyframe).arg(frame));

                const auto selectedKeyframes = scene()->selectedKeyframes();
                for (auto keyframe : selectedKeyframes) {
                    if (keyframe != current)
                        keyframe->commitPosition(mapToItem(current, keyframe->rect().center()));
                }
            }
        });
    }
}

void TimelineMoveTool::mouseDoubleClickEvent(TimelineMovableAbstractItem *item,
                                             QGraphicsSceneMouseEvent *event)
{
    Q_UNUSED(item)
    Q_UNUSED(event)
}

void TimelineMoveTool::keyPressEvent(QKeyEvent *keyEvent)
{
    Q_UNUSED(keyEvent)
}

void TimelineMoveTool::keyReleaseEvent(QKeyEvent *keyEvent)
{
    Q_UNUSED(keyEvent)
}

} // namespace QmlDesigner
