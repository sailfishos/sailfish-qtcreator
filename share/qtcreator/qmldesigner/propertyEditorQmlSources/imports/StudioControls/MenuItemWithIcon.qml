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
import QtQuick.Templates 2.12 as T
import StudioTheme 1.0 as StudioTheme
import QtQuick.Controls 2.12

T.MenuItem {
    id: control

    property int labelSpacing: StudioTheme.Values.contextMenuLabelSpacing

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(
                        implicitBackgroundHeight + topInset + bottomInset,
                        implicitContentHeight + topPadding + bottomPadding,
                        implicitIndicatorHeight + topPadding + bottomPadding)

    padding: 0
    spacing: 0
    horizontalPadding: StudioTheme.Values.contextMenuHorizontalPadding
    action: Action {}

    contentItem: Item {
        Text {
            id: iconLabel
            text: control.checked ? StudioTheme.Constants.tickIcon : ""
            visible: true
            color: control.enabled ? StudioTheme.Values.themeTextColor : StudioTheme.Values.themeTextColorDisabled
            font.family: StudioTheme.Constants.iconFont.family
            font.pixelSize: StudioTheme.Values.myIconFontSize
            anchors.verticalCenter: parent.verticalCenter
        }

        Text {
            id: textLabel
            x: StudioTheme.Values.height
            text: control.text
            font: control.font
            color: control.enabled ? StudioTheme.Values.themeTextColor : StudioTheme.Values.themeTextColorDisabled
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    background: Rectangle {
        implicitWidth: iconLabel.implicitWidth + textLabel.implicitWidth + control.labelSpacing
                       + control.leftPadding + control.rightPadding
        implicitHeight: StudioTheme.Values.height
        x: StudioTheme.Values.border
        y: StudioTheme.Values.border
        width: control.menu.width - (StudioTheme.Values.border * 2)
        height: control.height - (StudioTheme.Values.border * 2)
        color: control.down ? control.palette.midlight : control.highlighted ? StudioTheme.Values.themeInteraction : "transparent"
    }
}
