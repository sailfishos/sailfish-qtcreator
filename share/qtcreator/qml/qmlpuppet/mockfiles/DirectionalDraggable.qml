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

import QtQuick 2.0
import QtQuick3D 1.15
import MouseArea3D 1.0

Model {
    id: rootModel

    property View3D view3D
    property alias color: material.emissiveColor
    property Node targetNode: null
    property bool dragging: mouseAreaYZ.dragging || mouseAreaXZ.dragging
    property bool active: false
    property MouseArea3D dragHelper: null

    readonly property bool hovering: mouseAreaYZ.hovering || mouseAreaXZ.hovering

    property vector3d _scenePosPressed
    property real _posPressed
    property vector3d _targetStartPos

    signal pressed(var mouseArea)
    signal dragged(var mouseArea, vector3d sceneRelativeDistance, real relativeDistance)
    signal released(var mouseArea, vector3d sceneRelativeDistance, real relativeDistance)

    materials: DefaultMaterial {
        id: material
        emissiveColor: "white"
        lighting: DefaultMaterial.NoLighting
    }

    function handlePressed(mouseArea, planePos)
    {
        if (!targetNode)
            return;

        var maskedPosition = Qt.vector3d(planePos.x, 0, 0);
        _posPressed = planePos.x;
        _scenePosPressed = mouseArea.dragHelper.mapPositionToScene(maskedPosition);
        _targetStartPos = mouseArea.pivotScenePosition(targetNode);
        pressed(mouseArea);
    }

    function calcRelativeDistance(mouseArea, planePos)
    {
        var maskedPosition = Qt.vector3d(planePos.x, 0, 0);
        var scenePointerPos = mouseArea.dragHelper.mapPositionToScene(maskedPosition);
        return scenePointerPos.minus(_scenePosPressed);
    }

    function handleDragged(mouseArea, planePos)
    {
        if (!targetNode)
            return;

        dragged(mouseArea, calcRelativeDistance(mouseArea, planePos), planePos.x - _posPressed);
    }

    function handleReleased(mouseArea, planePos)
    {
        if (!targetNode)
            return;

        released(mouseArea, calcRelativeDistance(mouseArea, planePos), planePos.x - _posPressed);
    }

    MouseArea3D {
        id: mouseAreaYZ
        view3D: rootModel.view3D
        x: 0
        y: -1.5
        width: 12
        height: 3
        eulerRotation: Qt.vector3d(0, 0, 90)
        grabsMouse: targetNode
        active: rootModel.active
        dragHelper: rootModel.dragHelper

        onPressed: rootModel.handlePressed(mouseAreaYZ, planePos)
        onDragged: rootModel.handleDragged(mouseAreaYZ, planePos)
        onReleased: rootModel.handleReleased(mouseAreaYZ, planePos)
    }

    MouseArea3D {
        id: mouseAreaXZ
        view3D: rootModel.view3D
        x: 0
        y: -1.5
        width: 12
        height: 3
        eulerRotation: Qt.vector3d(0, 90, 90)
        grabsMouse: targetNode
        active: rootModel.active
        dragHelper: rootModel.dragHelper

        onPressed: rootModel.handlePressed(mouseAreaXZ, planePos)
        onDragged: rootModel.handleDragged(mouseAreaXZ, planePos)
        onReleased: rootModel.handleReleased(mouseAreaXZ, planePos)
    }
}

