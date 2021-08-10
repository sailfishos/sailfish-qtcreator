/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Templates 2.12 as T
import StudioTheme 1.0 as StudioTheme

Row {
    id: myButtonRow

    property bool hover: actionIndicator.hover || myButtonRow.childHover
    property bool childHover: false

    property alias actionIndicator: actionIndicator

    property alias actionIndicatorVisible: actionIndicator.visible
    property real __actionIndicatorWidth: StudioTheme.Values.actionIndicatorWidth
    property real __actionIndicatorHeight: StudioTheme.Values.actionIndicatorHeight

    ActionIndicator {
        id: actionIndicator
        myControl: myButtonRow
        x: 0
        y: 0
        // + StudioTheme.Values.border on width because of negative spacing on the row
        width: actionIndicator.visible ? myButtonRow.__actionIndicatorWidth + StudioTheme.Values.border : 0
        height: actionIndicator.visible ? myButtonRow.__actionIndicatorHeight : 0
    }

    spacing: -StudioTheme.Values.border

    function hoverCallback() {
        var hover = false

        for (var i = 0; i < children.length; ++i) {
            if (children[i].hovered !== undefined)
                hover = hover || children[i].hovered
        }

        myButtonRow.childHover = hover
    }

    onHoverChanged: {
        for (var i = 0; i < children.length; ++i)
            if (children[i].globalHover !== undefined)
                children[i].globalHover = myButtonRow.hover
    }
}
