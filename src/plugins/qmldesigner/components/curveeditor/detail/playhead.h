/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Design Tooling
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

#include <QRectF>
#include <QTimer>

QT_BEGIN_NAMESPACE
class QPainter;
QT_END_NAMESPACE

namespace QmlDesigner {

class GraphicsView;

class Playhead
{
public:
    Playhead(GraphicsView *view);

    int currentFrame() const;

    void paint(QPainter *painter, GraphicsView *view) const;

    void setMoving(bool moving);

    void moveToFrame(int frame, GraphicsView *view);

    void resize(GraphicsView *view);

    bool mousePress(const QPointF &pos);

    bool mouseMove(const QPointF &pos, GraphicsView *view);

    void mouseRelease(GraphicsView *view);

private:
    void mouseMoveOutOfBounds(GraphicsView *view);

    int m_frame;

    bool m_moving;

    QRectF m_rect;

    QTimer m_timer;
};

} // End namespace QmlDesigner.
