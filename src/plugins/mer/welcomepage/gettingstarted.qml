/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

import QtQuick 1.1
import widgets 1.0

Rectangle {
    id: gettingStartedRoot
    width: 920
    height: 600

    PageCaption {
        id: pageCaption

        x: 32
        y: 8

        anchors.rightMargin: 16
        anchors.right: parent.right
        anchors.leftMargin: 16
        anchors.left: parent.left

        caption: qsTr("Getting Started")
    }

    Item {
        id: canvas

        width: 920
        height: 200

        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 0

        GettingStartedItem {
            x: 90
            y: 83

            title: qsTr("Building and running applications")
            description: qsTr("Create your first SailfishOS application and test the installation")
            number: 1
            imageUrl: "widgets/images/sailfish-icon-buildrun.png"
            onClicked: gettingStarted.openSplitHelp("qthelp://com.nokia.qtcreator.262/doc/creator-mer-application.html")
        }

        GettingStartedItem {
            x: 360
            y: 83

            title: qsTr("SailfishOS UI")
            description: qsTr("Explore the design principles and workflow of the SailfishOS user interface")
            number: 2
            imageUrl: "widgets/images/sailfish-icon-showcase.png"
            onClicked: gettingStarted.openUrl("http://www.sailfishos.org/")
        }

        GettingStartedItem {
            x: 635
            y: 83

            title: qsTr("Develop")
            description: qsTr("Read and experiment with the Sailfish Silica components")
            number: 3
            imageUrl: "widgets/images/sailfish-icon-guideline.png"
            onClicked: gettingStarted.openSplitHelp("qthelp://sailfish.silica.100/sailfishsilica/index.html");
        }

        Image {
            x: 300
            y: 172
            source: "widgets/images/arrowBig.png"
        }

        Image {
            x: 593
            y: 172
            source: "widgets/images/arrowBig.png"
        }

    }
}
