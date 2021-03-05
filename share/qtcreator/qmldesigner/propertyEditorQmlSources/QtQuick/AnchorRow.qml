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

import QtQuick 2.15
import HelperWidgets 2.0
import QtQuick.Layouts 1.15
import QtQuickDesignerTheme 1.0

import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme

RowLayout {
    id: anchorRow

    opacity: enabled ? 1 : 0.5

    property alias iconSource: iconLabel.icon

    property variant anchorMargin

    property alias targetName: targetComboBox.targetName
    property alias currentText: targetComboBox.currentText

    property alias relativeTarget: buttonRow.relativeTarget

    signal targetChanged

    property bool verticalAnchor

    property bool invertRelativeTargets: false

    property bool showAlternativeTargets: true

    signal sameEdgeButtonClicked
    signal centerButtonClicked
    signal oppositeEdgeButtonClicked

    property alias buttonRow: buttonRow

    readonly property color __defaultTextColor: StudioTheme.Values.themeTextColor

    IconLabel {
        id: iconLabel
        Layout.alignment: Qt.AlignTop
        Layout.topMargin: 4
        Layout.leftMargin: 4
    }

    GridLayout {
        Layout.fillWidth: true
        rows: 2
        columns: 2

        Label {
            text: qsTr("Target")
            color: __defaultTextColor
            elide: Text.ElideRight
            Layout.minimumWidth: Math.min(60, Layout.preferredWidth + 10)
        }

        TargetComboBox {
            id: targetComboBox
            onCurrentTextChanged: {
                anchorRow.targetChanged();
            }
        }

        Label {
            text: qsTr("Margin")
            color: __defaultTextColor
            elide: Text.ElideRight
            Layout.minimumWidth: Math.min(60, Layout.preferredWidth + 10)
        }

        RowLayout {
            SpinBox {
                maximumValue: 0xffff
                minimumValue: -0xffff
                backendValue: anchorMargin
                realDragRange: 5000
            }

            StudioControls.ButtonRow {
                id: buttonRow
                actionIndicatorVisible: false

                property variant relativeTarget: anchorBackend.relativeAnchorTargetTop

                onRelativeTargetChanged: {

                    buttonSameEdge.checked = false
                    buttonCenter.checked = false
                    buttonOppositeEdge.checked = false

                    if (relativeTarget == AnchorBindingProxy.SameEdge) {
                        if (!invertRelativeTargets) {
                            buttonSameEdge.checked = true
                        } else {
                            buttonOppositeEdge.checked = true
                        }
                    } else if (relativeTarget == AnchorBindingProxy.OppositeEdge) {
                        if (!invertRelativeTargets) {
                            buttonOppositeEdge.checked = true
                        } else {
                            buttonSameEdge.checked = true
                        }
                    } else if (relativeTarget == AnchorBindingProxy.Center) {
                        buttonCenter.checked = true
                    }
                }

                StudioControls.ButtonGroup {
                    id: group
                }

                AbstractButton {
                    id: buttonSameEdge
                    buttonIcon: verticalAnchor ? StudioTheme.Constants.anchorTop : StudioTheme.Constants.anchorLeft
                    checkable: true
                    autoExclusive: true
                    StudioControls.ButtonGroup.group: group
                    tooltip: verticalAnchor ? qsTr("Anchor to the top of the target.") : qsTr("Anchor to the left of the target.")
                    onClicked: {
                        if (!invertRelativeTargets)
                            sameEdgeButtonClicked();
                        else
                            oppositeEdgeButtonClicked();
                    }
                }
                AbstractButton {
                    id: buttonCenter
                    buttonIcon: verticalAnchor ? StudioTheme.Constants.centerVertical : StudioTheme.Constants.centerHorizontal
                    checkable: true
                    autoExclusive: true
                    StudioControls.ButtonGroup.group: group
                    tooltip: verticalAnchor ? qsTr("Anchor to the vertical center of the target.") : qsTr("Anchor to the horizontal center of the target.")
                    onClicked: centerButtonClicked();
                }
                AbstractButton {
                    id: buttonOppositeEdge
                    buttonIcon: verticalAnchor ? StudioTheme.Constants.anchorBottom : StudioTheme.Constants.anchorRight
                    checkable: true
                    autoExclusive: true
                    StudioControls.ButtonGroup.group: group
                    tooltip: verticalAnchor ? qsTr("Anchor to the bottom of the target.") : qsTr("Anchor to the right of the target.")
                    onClicked: {
                        if (!invertRelativeTargets)
                            oppositeEdgeButtonClicked();
                        else
                            sameEdgeButtonClicked();
                    }
                }
            }


/*
            ButtonRow {
                id: buttonRow

                property variant relativeTarget: anchorBackend.relativeAnchorTargetTop

                onRelativeTargetChanged: {
                    if (relativeTarget == AnchorBindingProxy.SameEdge) {
                        if (!invertRelativeTargets) {
                            buttonRow.initalChecked = 0
                            buttonRow.checkedIndex = 0
                        } else {
                            buttonRow.initalChecked = 2
                            buttonRow.checkedIndex = 2
                        }
                    } else if (relativeTarget == AnchorBindingProxy.OppositeEdge) {
                        if (!invertRelativeTargets) {
                            buttonRow.initalChecked = 2
                            buttonRow.checkedIndex = 2
                        } else {
                            buttonRow.initalChecked = 0
                            buttonRow.checkedIndex = 0
                        }
                    } else if (relativeTarget == AnchorBindingProxy.Center) {
                        buttonRow.initalChecked = 1
                        buttonRow.checkedIndex = 1
                    }
                }

                exclusive: true
                ButtonRowButton {
                    iconSource: "image://icons/anchor-" + (verticalAnchor ? "top" : "left")
                    tooltip: verticalAnchor ? qsTr("Anchor to the top of the target.") : qsTr("Anchor to the left of the target.")
                    onClicked: {
                        if (!invertRelativeTargets)
                            sameEdgeButtonClicked();
                        else
                            oppositeEdgeButtonClicked();
                    }
                }

                ButtonRowButton {
                    iconSource: "image://icons/anchor-" + (verticalAnchor ? "vertical" : "horizontal")
                    tooltip: verticalAnchor ? qsTr("Anchor to the vertical center of the target.") : qsTr("Anchor to the horizontal center of the target.")

                    onClicked: centerButtonClicked();
                }

                ButtonRowButton {
                    iconSource: "image://icons/anchor-" + (verticalAnchor ? "bottom" : "right")
                    tooltip: verticalAnchor ? qsTr("Anchor to the bottom of the target.") : qsTr("Anchor to the right of the target.")
                    onClicked: {
                        if (!invertRelativeTargets)
                            oppositeEdgeButtonClicked();
                        else
                            sameEdgeButtonClicked();
                    }
                }
            }
*/
        }
    }
}
