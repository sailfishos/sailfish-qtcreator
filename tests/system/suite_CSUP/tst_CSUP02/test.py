############################################################################
#
# Copyright (C) 2016 The Qt Company Ltd.
# Contact: https://www.qt.io/licensing/
#
# This file is part of Qt Creator.
#
# Commercial License Usage
# Licensees holding valid commercial Qt licenses may use this file in
# accordance with the commercial license agreement provided with the
# Software or, alternatively, in accordance with the terms contained in
# a written agreement between you and The Qt Company. For licensing terms
# and conditions see https://www.qt.io/terms-conditions. For further
# information use the contact form at https://www.qt.io/contact-us.
#
# GNU General Public License Usage
# Alternatively, this file may be used under the terms of the GNU
# General Public License version 3 as published by the Free Software
# Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
# included in the packaging of this file. Please review the following
# information to ensure the GNU General Public License requirements will
# be met: https://www.gnu.org/licenses/gpl-3.0.html.
#
############################################################################

source("../../shared/qtcreator.py")

import time

def delayedType(editor, text):
    for c in text:
        type(editor, c)
        time.sleep(0.1)

# entry of test
def main():
    for useClang in [False, True]:
        with TestSection(getCodeModelString(useClang)):
            if not startCreatorVerifyingClang(useClang):
                continue
            # create qt quick application
# Step 1: Open test .pro project.
            createNewQtQuickApplication(tempDir(), "SampleApp")
            checkCodeModelSettings(useClang)
# Step 2: Open .cpp file in Edit mode.
            if not openDocument("SampleApp.Sources.main\\.cpp"):
                test.fatal("Could not open main.cpp")
                invokeMenuItem("File", "Exit")
                return
            test.verify(checkIfObjectExists(":Qt Creator_CppEditor::Internal::CPPEditorWidget"),
                        "Step 2: Verifying if: .cpp file is opened in Edit mode.")
# Steps 3&4: Insert text "class" to new line in Editor mode and press Ctrl+Space.
# Focus "class derived from QObject" in the list and press Tab or Enter to complete the code.
            editorWidget = findObject(":Qt Creator_CppEditor::Internal::CPPEditorWidget")
            mouseClick(editorWidget)
            jumpToFirstLine(editorWidget)
            type(editorWidget, "<Return>")
            type(editorWidget, "<Up>")
            delayedType(editorWidget, "class")
            if useClang:
                snooze(4)
            type(editorWidget, "<Ctrl+Space>")
            listView = waitForObject(":popupFrame_Proposal_QListView")
            shownProposals = dumpItems(listView.model())
            usedProposal = "class derived from QObject"
            expectedProposals = ["class", "class ", "class template",
                                 usedProposal, "class derived from QWidget"]
            test.xcompare(len(shownProposals), len(expectedProposals),  # QTCREATORBUG-23159
                          "Number of proposed templates")
            test.verify(set(expectedProposals).issubset(set(shownProposals)),
                        "Expected proposals shown, ignoring order?")
            doubleClickItem(listView, usedProposal, 5, 5, 0, Qt.LeftButton)
            pattern = ("(?<=class)\s+name\s*:\s*public\s+QObject\s*\{\s*Q_OBJECT\s+"
                       "public:\s+name\(\)\s*\{\}\s+virtual\s+~name\(\)\s*\{\}\s+\};")
            test.verify(re.search(pattern, str(editorWidget.plainText)),
                        "Code with several variables is inserted?")
# Step 5: Press Tab to move between the variables and specify values for them. For example write "Myname" for variable "name".
            type(editorWidget, "<Tab>")
            type(editorWidget, "<Tab>")
            type(editorWidget, "<Tab>")
            type(editorWidget, "Myname")
            result = re.search(pattern.replace("name", "Myname"), str(editorWidget.plainText))
            if result:
                test.passes("Step 5: Verifying if: A value for a variable is inserted and all "
                            "instances of the variable within the snippet are renamed.")
            else:
                test.fail("Step 5: Seems that not all instances of variable had been renamed "
                          "- Content of editor:\n%s" % editorWidget.plainText)
            invokeMenuItem('File', 'Revert "main.cpp" to Saved')
            clickButton(waitForObject(":Revert to Saved.Proceed_QPushButton"))
            invokeMenuItem("File", "Exit")
            waitForCleanShutdown()
