/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include <QtGlobal>

QT_FORWARD_DECLARE_CLASS(QGraphicsSceneMouseEvent)
QT_FORWARD_DECLARE_CLASS(QKeyEvent)
QT_FORWARD_DECLARE_CLASS(QPointF)

namespace QmlDesigner {

enum class ToolType { Move, Select };

class TimelineMovableAbstractItem;
class TimelineGraphicsScene;
class TimelineToolDelegate;

class TimelineAbstractTool
{
public:
    explicit TimelineAbstractTool(TimelineGraphicsScene *scene);
    explicit TimelineAbstractTool(TimelineGraphicsScene *scene, TimelineToolDelegate *delegate);
    virtual ~TimelineAbstractTool();

    virtual void mousePressEvent(TimelineMovableAbstractItem *item, QGraphicsSceneMouseEvent *event) = 0;
    virtual void mouseMoveEvent(TimelineMovableAbstractItem *item, QGraphicsSceneMouseEvent *event) = 0;
    virtual void mouseReleaseEvent(TimelineMovableAbstractItem *item,
                                   QGraphicsSceneMouseEvent *event) = 0;
    virtual void mouseDoubleClickEvent(TimelineMovableAbstractItem *item,
                                       QGraphicsSceneMouseEvent *event) = 0;

    virtual void keyPressEvent(QKeyEvent *keyEvent) = 0;
    virtual void keyReleaseEvent(QKeyEvent *keyEvent) = 0;

    TimelineGraphicsScene *scene() const;

    TimelineToolDelegate *delegate() const;

    QPointF startPosition() const;

    TimelineMovableAbstractItem *currentItem() const;

private:
    TimelineGraphicsScene *m_scene;

    TimelineToolDelegate *m_delegate;
};

} // namespace QmlDesigner
