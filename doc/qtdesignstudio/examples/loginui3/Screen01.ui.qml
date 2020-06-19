
/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/
import QtQuick 2.10
import loginui3 1.0
import QtQuick.Controls 2.3

Rectangle {
    id: root
    state: "loginState"
    width: Constants.width
    height: Constants.height

    Rectangle {
        id: loginPage
        color: "#ffffff"
        anchors.fill: parent

        Image {
            id: logo
            width: 100
            height: 100
            anchors.topMargin: 10
            anchors.left: parent.left
            anchors.leftMargin: 10
            anchors.top: parent.top
            source: "qt_logo_green_64x64px.png"
            fillMode: Image.PreserveAspectFit
        }

        Text {
            id: pageTitle
            width: 123
            height: 40
            text: qsTr("Qt Account")
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: 70
            font.pixelSize: 24
        }

        PushButton {
            id: backButton
            width: 40
            text: "<"
            font.pixelSize: 24
            anchors.right: parent.right
            anchors.rightMargin: 10
            anchors.top: parent.top
            anchors.topMargin: 20
        }

        Column {
            id: fieldColumn
            width: 300
            height: 130
            spacing: 5
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: 200

            TextField {
                id: usernameField
                width: 300
                placeholderText: qsTr("Username")
                font.pointSize: 10
            }

            TextField {
                id: passwordField
                width: 300
                placeholderText: qsTr("Password")
                font.pointSize: 10
            }

            TextField {
                id: verifyPasswordField
                width: 300
                visible: true
                font.pointSize: 10
                placeholderText: qsTr("Verify password")
            }
        }

        Column {
            id: buttonColumn
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 50
            spacing: 5
            anchors.horizontalCenter: parent.horizontalCenter

            PushButton {
                id: loginButton
                width: 120
                text: qsTr("Log In")
            }

            PushButton {
                id: registerButton
                width: 120
                text: qsTr("Create Account")
            }
        }
    }

    Connections {
        target: registerButton
        onClicked: {
            root.state = "registerState"
        }
    }

    Connections {
        target: backButton
        onClicked: {
            root.state = "loginState"
        }
    }
    states: [
        State {
            name: "loginState"

            PropertyChanges {
                target: verifyPasswordField
                visible: false
            }

            PropertyChanges {
                target: backButton
                visible: false
            }
        },
        State {
            name: "registerState"
            PropertyChanges {
                target: verifyPasswordField
                visible: true
            }

            PropertyChanges {
                target: loginButton
                visible: false
            }
        }
    ]
}
