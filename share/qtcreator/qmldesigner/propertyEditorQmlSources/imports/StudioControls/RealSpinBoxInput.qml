/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

TextInput {
    id: textInput

    property T.Control myControl

    property bool edit: textInput.activeFocus
    property bool drag: false

    z: 2
    font: myControl.font
    color: StudioTheme.Values.themeTextColor
    selectionColor: StudioTheme.Values.themeTextSelectionColor
    selectedTextColor: StudioTheme.Values.themeTextSelectedTextColor

    horizontalAlignment: Qt.AlignRight
    verticalAlignment: Qt.AlignVCenter
    leftPadding: StudioTheme.Values.inputHorizontalPadding
    rightPadding: StudioTheme.Values.inputHorizontalPadding

    readOnly: !myControl.editable
    validator: myControl.validator
    inputMethodHints: myControl.inputMethodHints
    selectByMouse: false
    activeFocusOnPress: false
    clip: true

    // TextInput focus needs to be set to activeFocus whenever it changes,
    // otherwise TextInput will get activeFocus whenever the parent SpinBox gets
    // activeFocus. This will lead to weird side effects.
    onActiveFocusChanged: textInput.focus = activeFocus

    Rectangle {
        id: textInputArea
        color: StudioTheme.Values.themeControlBackground
        border.color: StudioTheme.Values.themeControlOutline
        border.width: StudioTheme.Values.border
        x: 0
        y: 0
        z: -1
        width: textInput.width
        height: StudioTheme.Values.height
    }

    Item {
        id: dragModifierWorkaround
        Keys.onPressed: {
            event.accepted = true

            if (event.modifiers & Qt.ControlModifier) {
                mouseArea.stepSize = myControl.minStepSize
                mouseArea.calcValue(myControl.realValueModified)
            }

            if (event.modifiers & Qt.ShiftModifier) {
                mouseArea.stepSize = myControl.maxStepSize
                mouseArea.calcValue(myControl.realValueModified)
            }
        }
        Keys.onReleased: {
            event.accepted = true
            mouseArea.stepSize = myControl.realStepSize
            mouseArea.calcValue(myControl.realValueModified)
        }
    }

    // Ensure that we get Up and Down key press events first
    Keys.onShortcutOverride: {
        event.accepted = (event.key === Qt.Key_Up || event.key === Qt.Key_Down)
    }

    MouseArea {
        id: mouseArea

        property real stepSize: myControl.realStepSize

        property bool dragging: false
        property bool wasDragging: false
        property bool potentialDragStart: false

        property real initialValue: myControl.realValue

        property real pressStartX: 0.0
        property real dragStartX: 0.0
        property real translationX: 0.0

        anchors.fill: parent
        enabled: true
        hoverEnabled: true
        propagateComposedEvents: true
        acceptedButtons: Qt.LeftButton
        cursorShape: Qt.PointingHandCursor
        // Sets the global hover
        onContainsMouseChanged: myControl.hover = containsMouse

        onPositionChanged: {
            if (!mouseArea.dragging
                    && !myControl.edit
                    && Math.abs(mouseArea.pressStartX - mouse.x) > StudioTheme.Values.dragThreshold
                    && mouse.buttons === 1
                    && mouseArea.potentialDragStart) {
                mouseArea.dragging = true
                mouseArea.potentialDragStart = false
                mouseArea.initialValue = myControl.realValue
                mouseArea.cursorShape = Qt.ClosedHandCursor
                mouseArea.dragStartX = mouseArea.mouseX

                myControl.drag = true
                myControl.dragStarted()
                // Force focus on the non visible component to receive key events
                dragModifierWorkaround.forceActiveFocus()
                textInput.deselect()
            }

            if (!mouseArea.dragging)
                return

            mouse.accepted = true

            mouseArea.translationX += (mouseArea.mouseX - mouseArea.dragStartX)
            mouseArea.calcValue(myControl.realValueModified)
        }

        onCanceled: mouseArea.endDrag()

        onClicked: {
            if (textInput.edit)
                mouse.accepted = false

            if (mouseArea.wasDragging) {
                mouseArea.wasDragging = false
                return
            }

            textInput.forceActiveFocus()
            textInput.deselect() // QTBUG-75862
        }

        onPressed: {
            if (textInput.edit)
                mouse.accepted = false

            mouseArea.potentialDragStart = true
            mouseArea.pressStartX = mouseArea.mouseX
        }

        onReleased: {
            if (textInput.edit)
                mouse.accepted = false

            mouseArea.endDrag()
        }

        function endDrag() {
            if (!mouseArea.dragging)
                return

            mouseArea.dragging = false
            mouseArea.wasDragging = true

            if (myControl.compressedValueTimer.running) {
                myControl.compressedValueTimer.stop()
                mouseArea.calcValue(myControl.compressedRealValueModified)
            }
            mouseArea.cursorShape = Qt.PointingHandCursor
            myControl.drag = false
            myControl.dragEnded()
            // Avoid active focus on the component after dragging
            dragModifierWorkaround.focus = false
            textInput.focus = false
            myControl.focus = false

            mouseArea.translationX = 0
        }

        function calcValue(callback) {
            var minTranslation = (myControl.realFrom - mouseArea.initialValue) / mouseArea.stepSize
            var maxTranslation = (myControl.realTo - mouseArea.initialValue) / mouseArea.stepSize

            mouseArea.translationX = Math.min(Math.max(mouseArea.translationX, minTranslation), maxTranslation)

            myControl.setRealValue(mouseArea.initialValue + (mouseArea.translationX * mouseArea.stepSize))

            if (mouseArea.dragging)
                myControl.dragging()

            callback()
        }

        onWheel: {
            if (!myControl.__wheelEnabled)
                return

            // Set stepSize according to used modifier key
            if (wheel.modifiers & Qt.ControlModifier)
                mouseArea.stepSize = myControl.minStepSize

            if (wheel.modifiers & Qt.ShiftModifier)
                mouseArea.stepSize = myControl.maxStepSize

            myControl.valueFromText(textInput.text, myControl.locale)
            myControl.setRealValue(myControl.realValue + (wheel.angleDelta.y / 120.0 * mouseArea.stepSize))
            myControl.realValueModified()

            // Reset stepSize
            mouseArea.stepSize = myControl.realStepSize
        }
    }

    states: [
        State {
            name: "default"
            when: myControl.enabled && !textInput.edit
                  && !mouseArea.containsMouse && !myControl.drag
            PropertyChanges {
                target: textInputArea
                color: StudioTheme.Values.themeControlBackground
                border.color: StudioTheme.Values.themeControlOutline
            }
            PropertyChanges {
                target: mouseArea
                cursorShape: Qt.PointingHandCursor
            }
        },
        State {
            name: "hovered"
            when: myControl.hover && !textInput.edit && !myControl.drag
            PropertyChanges {
                target: textInputArea
                color: StudioTheme.Values.themeHoverHighlight
                border.color: StudioTheme.Values.themeControlOutline
            }
        },
        State {
            name: "edit"
            when: textInput.edit && !myControl.drag
            PropertyChanges {
                target: textInputArea
                color: StudioTheme.Values.themeFocusEdit
                border.color: StudioTheme.Values.themeInteraction
            }
            PropertyChanges {
                target: mouseArea
                cursorShape: Qt.IBeamCursor
            }
        },
        State {
            name: "drag"
            when: myControl.drag
            PropertyChanges {
                target: textInputArea
                color: StudioTheme.Values.themeFocusDrag
                border.color: StudioTheme.Values.themeInteraction
            }
        },
        State {
            name: "disabled"
            when: !myControl.enabled
            PropertyChanges {
                target: textInputArea
                color: StudioTheme.Values.themeControlBackgroundDisabled
                border.color: StudioTheme.Values.themeControlOutlineDisabled
            }
            PropertyChanges {
                target: textInput
                color: StudioTheme.Values.themeTextColorDisabled
            }
        }
    ]
}
