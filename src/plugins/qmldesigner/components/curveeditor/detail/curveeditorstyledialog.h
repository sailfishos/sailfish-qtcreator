
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

#include <QDialog>

QT_BEGIN_NAMESPACE
class QPushButton;
class QSpinBox;
class QDoubleSpinBox;
QT_END_NAMESPACE

namespace QmlDesigner {

namespace StyleEditor {
class ColorControl;
}

struct CurveEditorStyle;

class CurveEditorStyleDialog : public QDialog
{
    Q_OBJECT

signals:
    void styleChanged(const CurveEditorStyle &style);

public:
    CurveEditorStyleDialog(CurveEditorStyle &style, QWidget *parent = nullptr);

    CurveEditorStyle style() const;

private:
    void emitStyleChanged();

    void printStyle();

private:
    QPushButton *m_printButton;

    StyleEditor::ColorControl *m_background;

    StyleEditor::ColorControl *m_backgroundAlternate;

    StyleEditor::ColorControl *m_fontColor;

    StyleEditor::ColorControl *m_gridColor;

    QDoubleSpinBox *m_canvasMargin;

    QSpinBox *m_zoomInWidth;

    QSpinBox *m_zoomInHeight;

    QDoubleSpinBox *m_timeAxisHeight;

    QDoubleSpinBox *m_timeOffsetLeft;

    QDoubleSpinBox *m_timeOffsetRight;

    StyleEditor::ColorControl *m_rangeBarColor;

    StyleEditor::ColorControl *m_rangeBarCapsColor;

    QDoubleSpinBox *m_valueAxisWidth;

    QDoubleSpinBox *m_valueOffsetTop;

    QDoubleSpinBox *m_valueOffsetBottom;

    // HandleItem
    QDoubleSpinBox *m_handleSize;

    QDoubleSpinBox *m_handleLineWidth;

    StyleEditor::ColorControl *m_handleColor;

    StyleEditor::ColorControl *m_handleSelectionColor;

    // KeyframeItem
    QDoubleSpinBox *m_keyframeSize;

    StyleEditor::ColorControl *m_keyframeColor;

    StyleEditor::ColorControl *m_keyframeSelectionColor;

    // CurveItem
    QDoubleSpinBox *m_curveWidth;

    StyleEditor::ColorControl *m_curveColor;

    StyleEditor::ColorControl *m_curveSelectionColor;

    // TreeItem
    QDoubleSpinBox *m_treeMargins;

    // Playhead
    QDoubleSpinBox *m_playheadWidth;

    QDoubleSpinBox *m_playheadRadius;

    StyleEditor::ColorControl *m_playheadColor;
};

} // End namespace QmlDesigner.
