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

import QtQuick 2.1
import HelperWidgets 2.0
import QtQuick.Layouts 1.0

Column {
    anchors.left: parent.left
    anchors.right: parent.right

    FlickableSection {
        anchors.left: parent.left
        anchors.right: parent.right
    }

    Section {
        anchors.left: parent.left
        anchors.right: parent.right
        caption: qsTr("Grid View")

        SectionLayout {

            Label {
                text: qsTr("Cache")
                tooltip: qsTr("Cache buffer")
            }

            SectionLayout {
                SpinBox {
                    backendValue: backendValues.cacheBuffer
                    minimumValue: 0
                    maximumValue: 1000
                    decimals: 0
                }
                ExpandingSpacer {
                }
            }

            Label {
                text: qsTr("Cell Size")
            }

            SecondColumnLayout {
                Label {
                    text: "W"
                    width: 12
                }
                SpinBox {
                    backendValue: backendValues.cellWidth
                    minimumValue: 0
                    maximumValue: 1000
                    decimals: 0
                }

                Item {
                    width: 4
                    height: 4
                }

                Label {
                    text: "H"
                    width: 12
                }
                SpinBox {
                    backendValue: backendValues.cellHeight
                    minimumValue: 0
                    maximumValue: 1000
                    decimals: 0
                }
                ExpandingSpacer {
                }
            }

            Label {
                text: qsTr("Flow")
            }

            SecondColumnLayout {
                ComboBox {
                    scope: "GridView"
                    model: ["FlowLeftToRight", "FlowTopToBottom"]
                    backendValue: backendValues.flow
                    Layout.fillWidth: true
                }
                ExpandingSpacer {
                }
            }

            Label {
                text: qsTr("Navigation wraps")
                tooltip: qsTr("Determines whether the grid wraps key navigation.")
            }

            SectionLayout {
                CheckBox {
                    Layout.fillWidth: true
                    backendValue: backendValues.keyNavigationWraps
                    text: backendValues.keyNavigationWraps.valueToString
                }
                ExpandingSpacer {
                }
            }

            Label {
                text: qsTr("Layout Direction")
            }

            SecondColumnLayout {
                ComboBox {
                    scope: "Qt"
                    model: ["LeftToRight", "RightToLeft"]
                    backendValue: backendValues.layoutDirection
                    Layout.fillWidth: true
                }
                ExpandingSpacer {
                }
            }

            Label {
                text: qsTr("Snap mode")
                tooltip: qsTr("Determines how the view scrolling will settle following a drag or flick.")
            }

            SecondColumnLayout {
                ComboBox {
                    scope: "GridView"
                    model: ["NoSnap", "SnapToRow", "SnapOneRow"]
                    backendValue: backendValues.snapMode
                    Layout.fillWidth: true
                }
                ExpandingSpacer {
                }
            }

        }
    }

    Section {
        anchors.left: parent.left
        anchors.right: parent.right
        caption: qsTr("Grid View Highlight")

        SectionLayout {

            Label {
                text: qsTr("Range")
                tooltip: qsTr("Highlight range")
            }

            SecondColumnLayout {
                ComboBox {
                    scope: "GridView"
                    model: ["NoHighlightRange", "ApplyRange", "StrictlyEnforceRange"]
                    backendValue: backendValues.highlightRangeMode
                    Layout.fillWidth: true
                }
                ExpandingSpacer {
                }
            }

            Label {
                text: qsTr("Move duration")
                tooltip: qsTr("Move animation duration of the highlight delegate.")
            }

            SectionLayout {
                SpinBox {
                    backendValue: backendValues.highlightMoveDuration
                    minimumValue: 0
                    maximumValue: 1000
                    decimals: 0
                }
                ExpandingSpacer {
                }
            }

            Label {
                text: qsTr("Move speed")
                tooltip: qsTr("Move animation speed of the highlight delegate.")
            }

            SectionLayout {
                SpinBox {
                    backendValue: backendValues.highlightMoveSpeed
                    minimumValue: 0
                    maximumValue: 1000
                    decimals: 0
                }
                ExpandingSpacer {
                }
            }

            Label {
                text: qsTr("Preferred begin")
                tooltip: qsTr("Preferred highlight begin - must be smaller than Preferred end.")
            }

            SectionLayout {
                SpinBox {
                    backendValue: backendValues.preferredHighlightBegin
                    minimumValue: 0
                    maximumValue: 1000
                    decimals: 0
                }
                ExpandingSpacer {
                }
            }

            Label {
                text: qsTr("Preferred end")
                tooltip: qsTr("Preferred highlight end - must be larger than Preferred begin.")
            }

            SectionLayout {
                SpinBox {
                    backendValue: backendValues.preferredHighlightEnd
                    minimumValue: 0
                    maximumValue: 1000
                    decimals: 0
                }
                ExpandingSpacer {
                }
            }

            Label {
                text: qsTr("Follows current")
                tooltip: qsTr("Determines whether the highlight is managed by the view.")
            }

            SectionLayout {
                CheckBox {
                    Layout.fillWidth: true
                    backendValue: backendValues.highlightFollowsCurrentItem
                    text: backendValues.highlightFollowsCurrentItem.valueToString
                }
                ExpandingSpacer {
                }
            }

        }
    }
}
