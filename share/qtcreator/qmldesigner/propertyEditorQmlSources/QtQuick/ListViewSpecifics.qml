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
        caption: qsTr("List View")

        SectionLayout {

            Label {
                text: qsTr("Cache")
                tooltip: qsTr("Cache buffer")
                disabledState: !backendValues.cacheBuffer.isAvailable
            }

            SectionLayout {
                SpinBox {
                    backendValue: backendValues.cacheBuffer
                    minimumValue: 0
                    maximumValue: 1000
                    decimals: 0
                    enabled: backendValue.isAvailable
                }

                ExpandingSpacer {
                }
            }

            Label {
                text: qsTr("Navigation wraps")
                tooltip: qsTr("Whether the grid wraps key navigation.")
                disabledState: !backendValues.keyNavigationWraps.isAvailable
            }

            SectionLayout {
                CheckBox {
                    Layout.fillWidth: true
                    backendValue: backendValues.keyNavigationWraps
                    text: backendValues.keyNavigationWraps.valueToString
                    enabled: backendValue.isAvailable
                }
                ExpandingSpacer {
                }
            }

            Label {
                text: qsTr("Orientation")
                tooltip: qsTr("Orientation of the list.")
            }

            SecondColumnLayout {
                ComboBox {
                    scope: "ListView"
                    model: ["Horizontal", "Vertical"]
                    backendValue: backendValues.orientation
                    Layout.fillWidth: true
                }
                ExpandingSpacer {
                }
            }

            Label {
                text: qsTr("Layout direction")
                disabledState: !backendValues.layoutDirection.isAvailable
            }

            SecondColumnLayout {
                ComboBox {
                    scope: "Qt"
                    model: ["LeftToRight", "RightToLeft"]
                    backendValue: backendValues.layoutDirection
                    Layout.fillWidth: true
                    enabled: backendValue.isAvailable
                }
                ExpandingSpacer {
                }
            }

            Label {
                text: qsTr("Snap mode")
                tooltip: qsTr("Determines how the view scrolling will settle following a drag or flick.")
                disabledState: !backendValues.snapMode.isAvailable
            }

            SecondColumnLayout {
                ComboBox {
                    scope: "ListView"
                    model: ["NoSnap", "SnapToItem", "SnapOneItem"]
                    backendValue: backendValues.snapMode
                    Layout.fillWidth: true
                    enabled: backendValue.isAvailable
                }
                ExpandingSpacer {
                }
            }

            Label {
                text: qsTr("Spacing")
                tooltip: qsTr("Spacing between items.")
            }

            SectionLayout {
                SpinBox {
                    backendValue: backendValues.spacing
                    minimumValue: -4000
                    maximumValue: 4000
                    decimals: 0
                }
                ExpandingSpacer {
                }
            }

        }
    }

    Section {
        anchors.left: parent.left
        anchors.right: parent.right
        caption: qsTr("List View Highlight")

        SectionLayout {

            Label {
                text: qsTr("Range")
                tooltip: qsTr("Highlight range")
                disabledState: !backendValues.highlightRangeMode.isAvailable
            }

            SecondColumnLayout {
                ComboBox {
                    scope: "ListView"
                    model: ["NoHighlightRange", "ApplyRange", "StrictlyEnforceRange"]
                    backendValue: backendValues.highlightRangeMode
                    Layout.fillWidth: true
                    enabled: backendValue.isAvailable
                }
                ExpandingSpacer {
                }
            }


            Label {
                text: qsTr("Move duration")
                tooltip: qsTr("Move animation duration of the highlight delegate.")
                disabledState: !backendValues.highlightMoveDuration.isAvailable
            }

            SectionLayout {
                SpinBox {
                    backendValue: backendValues.highlightMoveDuration
                    minimumValue: -1
                    maximumValue: 1000
                    decimals: 0
                    enabled: backendValue.isAvailable
                }
                ExpandingSpacer {
                }
            }

            Label {
                text: qsTr("Move velocity")
                tooltip: qsTr("Move animation velocity of the highlight delegate.")
                disabledState: !backendValues.highlightMoveVelocity.isAvailable
            }

            SectionLayout {
                SpinBox {
                    backendValue: backendValues.highlightMoveVelocity
                    minimumValue: -1
                    maximumValue: 1000
                    decimals: 0
                    enabled: backendValue.isAvailable
                }
                ExpandingSpacer {
                }
            }

            Label {
                text: qsTr("Resize duration")
                tooltip: qsTr("Resize animation duration of the highlight delegate.")
                disabledState: !backendValues.highlightResizeDuration.isAvailable
            }

            SectionLayout {
                SpinBox {
                    backendValue: backendValues.highlightResizeDuration
                    minimumValue: -1
                    maximumValue: 1000
                    decimals: 0
                    enabled: backendValue.isAvailable
                }
                ExpandingSpacer {
                }
            }

            Label {
                text: qsTr("Resize velocity")
                tooltip: qsTr("Resize animation velocity of the highlight delegate.")
                disabledState: !backendValues.highlightResizeVelocity.isAvailable
            }

            SectionLayout {
                SpinBox {
                    backendValue: backendValues.highlightResizeVelocity
                    minimumValue: -1
                    maximumValue: 1000
                    decimals: 0
                    enabled: backendValue.isAvailable
                }
                ExpandingSpacer {
                }
            }

            Label {
                text: qsTr("Preferred begin")
                tooltip: qsTr("Preferred highlight begin - must be smaller than Preferred end.")
                disabledState: !backendValues.preferredHighlightBegin.isAvailable
            }

            SectionLayout {
                SpinBox {
                    backendValue: backendValues.preferredHighlightBegin
                    minimumValue: 0
                    maximumValue: 1000
                    decimals: 0
                    enabled: backendValue.isAvailable
                }
                ExpandingSpacer {
                }
            }

            Label {
                text: qsTr("Preferred end")
                tooltip: qsTr("Preferred highlight end - must be larger than Preferred begin.")
                disabledState: !backendValues.preferredHighlightEnd.isAvailable
            }

            SectionLayout {
                SpinBox {
                    backendValue: backendValues.preferredHighlightEnd
                    minimumValue: 0
                    maximumValue: 1000
                    decimals: 0
                    enabled: backendValue.isAvailable
                }
                ExpandingSpacer {
                }
            }

            Label {
                text: qsTr("Follows current")
                tooltip: qsTr("Whether the highlight is managed by the view.")
                disabledState: !backendValues.highlightFollowsCurrentItem.isAvailable
            }

            SectionLayout {
                CheckBox {
                    Layout.fillWidth: true
                    backendValue: backendValues.highlightFollowsCurrentItem
                    text: backendValues.highlightFollowsCurrentItem.valueToString
                    enabled: backendValue.isAvailable
                }
                ExpandingSpacer {
                }
            }

        }
    }
}
