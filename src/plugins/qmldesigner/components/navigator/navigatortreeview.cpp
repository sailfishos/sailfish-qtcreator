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

#include "navigatortreeview.h"

#include <qmath.h>

#include "navigatorview.h"
#include "navigatortreemodel.h"
#include "qproxystyle.h"
#include "previewtooltip.h"

#include <metainfo.h>
#include <theme.h>

#include <utils/icon.h>
#include <utils/utilsicons.h>

#include <QLineEdit>
#include <QPen>
#include <QPixmapCache>
#include <QMouseEvent>
#include <QPainter>
#include <QStyleFactory>
#include <QEvent>
#include <QImage>

namespace QmlDesigner {

namespace {

// This style basically allows us to span the entire row
// including the arrow indicators which would otherwise not be
// drawn by the delegate
class TableViewStyle : public QProxyStyle
{
public:
    TableViewStyle(QObject *parent) : QProxyStyle(QStyleFactory::create("fusion"))
    {
        setParent(parent);
        baseStyle()->setParent(parent);
    }

    void drawPrimitive(PrimitiveElement element,
                       const QStyleOption *option,
                       QPainter *painter,
                       const QWidget *widget = nullptr) const override
    {
        static QRect mouseOverStateSavedFrameRectangle;
        if (element == QStyle::PE_PanelItemViewRow) {
            if (option->state & QStyle::State_MouseOver)
                mouseOverStateSavedFrameRectangle = option->rect;

            painter->fillRect(option->rect.adjusted(0, delegateMargin, 0, -delegateMargin),
                              Theme::getColor(Theme::Color::QmlDesigner_BorderColor));
        } else if (element == PE_IndicatorItemViewItemDrop) {
            // between elements and on elements we have a width
            if (option->rect.width() > 0) {
                m_currentTextColor = option->palette.text().color();
                QRect frameRectangle = adjustedRectangleToWidgetWidth(option->rect, widget);
                painter->save();
                if (option->rect.height() == 0) {
                    bool isNotRootItem = option->rect.top() > 10 && mouseOverStateSavedFrameRectangle.top() > 10;
                    if (isNotRootItem)
                        drawIndicatorLine(frameRectangle.topLeft(), frameRectangle.topRight(), painter);
                } else {
                    drawHighlightFrame(frameRectangle, painter);
                }
                painter->restore();
            }
        } else if (element == PE_FrameFocusRect) {
            // don't draw
        } else if (element == PE_IndicatorBranch) {
            painter->save();
            static const int decoration_size = 10;
            int mid_h = option->rect.x() + option->rect.width() / 2;
            int mid_v = option->rect.y() + option->rect.height() / 2;
            int bef_h = mid_h;
            int bef_v = mid_v;
            int aft_h = mid_h;
            int aft_v = mid_v;
            QBrush brush(Theme::getColor(Theme::Color::DSsliderHandle), Qt::SolidPattern);
            if (option->state & State_Item) {
                if (option->direction == Qt::RightToLeft)
                    painter->fillRect(option->rect.left(), mid_v, bef_h - option->rect.left(), 1, brush);
                else
                    painter->fillRect(aft_h, mid_v, option->rect.right() - aft_h + 1, 1, brush);
            }
            if (option->state & State_Sibling)
                painter->fillRect(mid_h, aft_v, 1, option->rect.bottom() - aft_v + 1, brush);
            if (option->state & (State_Open | State_Children | State_Item | State_Sibling))
                painter->fillRect(mid_h, option->rect.y(), 1, bef_v - option->rect.y(), brush);
            if (option->state & State_Children) {
                int delta = decoration_size / 2;
                bef_h -= delta;
                bef_v -= delta;
                aft_h += delta;
                aft_v += delta;

                const QRectF rect(bef_h, bef_v, decoration_size + 1, decoration_size + 1);
                painter->fillRect(rect, QBrush(Theme::getColor(Theme::Color::QmlDesigner_BackgroundColorDarkAlternate)));

                static const QPointF collapsePoints[3] = {
                    QPointF(0.0, 0.0),
                    QPointF(4.0, 4.0),
                    QPointF(0.0, 8.0)
                };

                static const QPointF expandPoints[3] = {
                    QPointF(0.0, 0.0),
                    QPointF(8.0, 0.0),
                    QPointF(4.0, 4.0)
                };

                auto color = Theme::getColor(Theme::Color::IconsBaseColor);
                painter->setPen(color);
                painter->setBrush(color);

                if (option->state & QStyle::State_Open) {
                    painter->translate(QPointF(mid_h - 4, mid_v - 2));
                    painter->drawConvexPolygon(expandPoints, 3);
                } else {
                    painter->translate(QPointF(mid_h - 2, mid_v - 4));
                    painter->drawConvexPolygon(collapsePoints, 3);
                }
            }
            painter->restore();

        } else {
            QProxyStyle::drawPrimitive(element, option, painter, widget);
        }
    }

    int styleHint(StyleHint hint,
                  const QStyleOption *option = nullptr,
                  const QWidget *widget = nullptr,
                  QStyleHintReturn *returnData = nullptr) const override
    {
        if (hint == SH_ItemView_ShowDecorationSelected)
            return 0;
        else
            return QProxyStyle::styleHint(hint, option, widget, returnData);
    }

private: // functions
    QColor highlightBrushColor() const
    {
        QColor highlightBrushColor = m_currentTextColor;
        highlightBrushColor.setAlphaF(0.7);
        return highlightBrushColor;
    }
    QColor highlightLineColor() const
    {
        return highlightBrushColor().lighter();
    }
    QColor backgroundBrushColor() const
    {
        QColor backgroundBrushColor = highlightBrushColor();
        backgroundBrushColor.setAlphaF(0.2);
        return backgroundBrushColor;
    }
    QColor backgroundLineColor() const
    {
        return backgroundBrushColor().lighter();
    }

    void drawHighlightFrame(const QRect &frameRectangle, QPainter *painter) const
    {
        painter->setPen(QPen(highlightLineColor(), 2));
        painter->setBrush(highlightBrushColor());
        painter->drawRect(frameRectangle);
    }
    void drawBackgroundFrame(const QRect &frameRectangle, QPainter *painter) const
    {
        painter->setPen(QPen(backgroundLineColor(), 2));
        painter->setBrush(backgroundBrushColor());
        painter->drawRect(frameRectangle);
    }
    void drawIndicatorLine(const QPoint &leftPoint, const QPoint &rightPoint, QPainter *painter) const
    {
        painter->setPen(QPen(highlightLineColor(), 3));
        painter->drawLine(leftPoint, rightPoint);
    }

    QRect adjustedRectangleToWidgetWidth(const QRect &originalRectangle, const QWidget *widget) const
    {
        QRect adjustesRectangle = originalRectangle;
        adjustesRectangle.setLeft(0);
        adjustesRectangle.setWidth(widget->rect().width());
        return adjustesRectangle.adjusted(0, 0, -1, -1);
    }
private: // variables
    mutable QColor m_currentTextColor;
};

}

NavigatorTreeView::NavigatorTreeView(QWidget *parent)
    : QTreeView(parent)
{
    setStyle(new TableViewStyle(this));
    setMinimumWidth(240);
    setRootIsDecorated(false);
    setIndentation(indentation() * 0.5);
    viewport()->setAttribute(Qt::WA_Hover);
}

void NavigatorTreeView::drawSelectionBackground(QPainter *painter, const QStyleOption &option)
{
    painter->save();
    painter->fillRect(option.rect.adjusted(0, delegateMargin, 0, -delegateMargin),
                      Theme::getColor(Theme::Color::QmlDesigner_HighlightColor));
    painter->restore();
}

bool NavigatorTreeView::viewportEvent(QEvent *event)
{
    const QPoint offset(10, 5);

    if (event->type() == QEvent::ToolTip) {
        auto navModel = qobject_cast<NavigatorTreeModel *>(model());
        if (navModel) {
            QHelpEvent *helpEvent = static_cast<QHelpEvent *>(event);
            QModelIndex index = indexAt(helpEvent->pos());
            QVariantMap imgMap = navModel->data(index, ToolTipImageRole).toMap();

            if (!imgMap.isEmpty()) {
                m_previewToolTipNodeId = index.internalId();
                if (!m_previewToolTip || devicePixelRatioF() != m_previewToolTip->devicePixelRatioF()) {
                    if (!m_previewToolTip) {
                        connect(navModel, &NavigatorTreeModel::toolTipPixmapUpdated,
                                [this](const QString &id, const QPixmap &pixmap) {
                            if (m_previewToolTip && m_previewToolTip->id() == id)
                                m_previewToolTip->setPixmap(pixmap);
                        });
                    } else {
                        delete m_previewToolTip;
                    }
                    m_previewToolTip = new PreviewToolTip();
                }
                m_previewToolTip->setId(imgMap["id"].toString());
                m_previewToolTip->setType(imgMap["type"].toString());
                m_previewToolTip->setInfo(imgMap["info"].toString());
                m_previewToolTip->setPixmap(imgMap["pixmap"].value<QPixmap>());
                m_previewToolTip->move(helpEvent->globalPos() + offset);
                if (!m_previewToolTip->isVisible())
                    m_previewToolTip->show();
            } else if (m_previewToolTip) {
                m_previewToolTip->hide();
                m_previewToolTipNodeId = -1;
            }
        }
    } else if (event->type() == QEvent::Leave) {
        if (m_previewToolTip) {
            m_previewToolTip->hide();
            m_previewToolTipNodeId = -1;
        }
    } else if (event->type() == QEvent::HoverMove) {
        if (m_previewToolTip && m_previewToolTip->isVisible()) {
            auto *he = static_cast<QHoverEvent *>(event);
            QModelIndex index = indexAt(he->pos());
            if (!index.isValid() || index.internalId() != quintptr(m_previewToolTipNodeId)) {
                m_previewToolTip->hide();
                m_previewToolTipNodeId = -1;
            } else {
                m_previewToolTip->move(mapToGlobal(he->pos()) + offset);
            }
        }
    }

    return QTreeView::viewportEvent(event);
}

}
