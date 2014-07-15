/****************************************************************************
**
** Copyright (C) 2012 - 2013 Jolla Ltd.
** Contact: http://jolla.com/
**
** This file is part of Qt Creator.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Digia.
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
