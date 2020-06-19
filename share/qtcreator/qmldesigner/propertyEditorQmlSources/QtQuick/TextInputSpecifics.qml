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

    StandardTextSection {
        showVerticalAlignment: true
    }

    Section {
        anchors.left: parent.left
        anchors.right: parent.right
        caption: qsTr("Text Color")

        ColorEditor {
            caption: qsTr("Text Color")
            backendValue: backendValues.color
            supportGradient: false
        }

    }

    Section {
        anchors.left: parent.left
        anchors.right: parent.right
        caption: qsTr("Selection Color")

        ColorEditor {
            caption: qsTr("Selection Color")
            backendValue:  backendValues.selectionColor
            supportGradient: false
        }
    }

    Section {
        anchors.left: parent.left
        anchors.right: parent.right
        caption: qsTr("Selected Text Color")

        ColorEditor {
            caption: qsTr("Selected Text Color")
            backendValue: backendValues.selectedTextColor
            supportGradient: false
        }
    }

   FontSection {
       showStyle: false
   }

   TextInputSection {
       isTextInput: true
   }

   PaddingSection {
       visible: minorQtQuickVersion > 5
   }

}
