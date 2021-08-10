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
import StudioTheme 1.0 as StudioTheme

Item {
    id: section
    property alias caption: label.text
    property int leftPadding: 8
    property int topPadding: 4
    property int rightPadding: 0

    property int animationDuration: 120

    property bool expanded: true

    clip: true

    Rectangle {
        id: header
        height: StudioTheme.Values.sectionHeadHeight

        anchors.left: parent.left
        anchors.right: parent.right

        color: StudioTheme.Values.themeSectionHeadBackground

        SectionLabel {
            id: label
            anchors.verticalCenter: parent.verticalCenter
            color: StudioTheme.Values.themeTextColor
            x: 22
            font.pixelSize: StudioTheme.Values.myFontSize
            font.capitalization: Font.AllUppercase
        }

        SectionLabel {
            id: arrow
            width: StudioTheme.Values.spinControlIconSizeMulti
            height: StudioTheme.Values.spinControlIconSizeMulti
            text: StudioTheme.Constants.startNode
            color: StudioTheme.Values.themeTextColor
            renderType: Text.NativeRendering
            anchors.left: parent.left
            anchors.leftMargin: 4
            anchors.verticalCenter: parent.verticalCenter
            font.pixelSize: StudioTheme.Values.spinControlIconSizeMulti
            font.family: StudioTheme.Constants.iconFont.family
            Behavior on rotation {
                NumberAnimation {
                    easing.type: Easing.OutCubic
                    duration: section.animationDuration
                }
            }
        }

        MouseArea {
            anchors.fill: parent
            onClicked: {
                section.expanded = !section.expanded
                if (!section.expanded) // TODO
                    section.forceActiveFocus()
            }
        }
    }

    default property alias __content: row.children

    readonly property alias contentItem: row

    implicitHeight: Math.round(row.height + header.height + topRow.height + bottomRow.height)

    Row {
        id: topRow
        height: StudioTheme.Values.sectionHeadSpacerHeight
        anchors.top: header.bottom
    }

    Row {
        id: row
        anchors.left: parent.left
        anchors.leftMargin: leftPadding
        anchors.right: parent.right
        anchors.rightMargin: rightPadding
        anchors.top: topRow.bottom
    }

    Row {
        id: bottomRow
        height: StudioTheme.Values.sectionHeadSpacerHeight
        anchors.top: row.bottom
    }

    Behavior on implicitHeight {
        NumberAnimation {
            easing.type: Easing.OutCubic
            duration: section.animationDuration
        }
    }

    states: [
        State {
            name: "Expanded"
            when: section.expanded
            PropertyChanges {
                target: arrow
                rotation: 90
            }
        },
        State {
            name: "Collapsed"
            when: !section.expanded
            PropertyChanges {
                target: section
                implicitHeight: header.height
            }
            PropertyChanges {
                target: arrow
                rotation: 0
            }
        }
    ]
}
