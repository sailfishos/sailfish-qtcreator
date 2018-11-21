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
import __builtin__

cppEditorStr = ":Qt Creator_CppEditor::Internal::CPPEditorWidget"
originalSources = os.path.abspath(os.path.join(os.getcwd(), "..", "shared", "simplePlainCPP"))

def init():
    global homeShortCut, endShortCut
    if platform.system() == "Darwin":
        homeShortCut = "<Ctrl+Left>"
        endShortCut = "<End>"
    else:
        homeShortCut = "<Home>"
        endShortCut = "<Ctrl+End>"

def main():
    folder = prepareTemplate(originalSources)
    if folder == None:
        test.fatal("Could not prepare test files - leaving test")
        return
    proFile = os.path.join(folder, "testfiles.pro")
    startQC()
    if not startedWithoutPluginError():
        return
    openQmakeProject(proFile)
    fileModifications = {"testfiles.testfiles\\.pro":__modifyProFile__,
                         "testfiles.Headers.testfile\\.h":__modifyHeader__,
                         "testfiles.Sources.testfile\\.cpp":__modifySource__,
                         "testfiles.Sources.main\\.cpp":None}
    for fileName, modification in fileModifications.iteritems():
        __modifyFile__(fileName, modification)
    test.log("Reverting all files...")
    fileModifications = dict(zip(fileModifications.keys(),
                                 (__builtin__.bool(v) for v in fileModifications.values())))
    revertChanges(fileModifications)
    invokeMenuItem("File", "Exit")

def __modifyFile__(fileName, modificationFunc):
    simpleFName = simpleFileName(fileName)
    test.log("Opening file '%s'" % simpleFName)
    openDocument(fileName)
    if modificationFunc:
        test.log("Modifying file '%s'" % simpleFName)
        modificationFunc()
    else:
        test.log("Leaving file '%s' unmodified." % simpleFName)

# add some stuff to pro file
def __modifyProFile__():
    proEditorStr = ":Qt Creator_TextEditor::TextEditorWidget"
    addConfig = ["", "CONFIG += thread", "",
                 "lessThan(QT_VER_MAJ, 4) | lessThan(QT_VER_MIN, 7) {",
                 "    error(Qt 4.7 or newer is required but version $$[QT_VERSION] was detected.)",
                 "}"]
    addFile = [" \\", "    not_existing.cpp"]
    if placeCursorToLine(proEditorStr, "CONFIG -= qt"):
        typeLines(proEditorStr, addConfig)
    if placeCursorToLine(proEditorStr, "testfile.cpp"):
        typeLines(proEditorStr, addFile)

# re-order some stuff inside header
def __modifyHeader__():
    global cppEditorStr, homeShortCut, endShortCut
    if placeCursorToLine(cppEditorStr, "class.+", True):
        type(cppEditorStr, homeShortCut)
        markText(cppEditorStr, "Down", 5)
        invokeMenuItem("Edit", "Cut")
        type(cppEditorStr, endShortCut)
        type(cppEditorStr, "<Return>")
        invokeMenuItem("Edit", "Paste")

# remove some stuff from source
def __modifySource__():
    global cppEditorStr, homeShortCut
    if placeCursorToLine(cppEditorStr, "void function1(int a);"):
        type(cppEditorStr, homeShortCut)
        markText(cppEditorStr, "Down")
        type(cppEditorStr, "<Delete>")
    if placeCursorToLine(cppEditorStr, "bool function1(int a) {"):
        type(cppEditorStr, homeShortCut)
        markText(cppEditorStr, "Down", 4)
        type(cppEditorStr, "<Delete>")

def revertChanges(files):
    for f,canRevert in files.iteritems():
        simpleName = simpleFileName(f)
        test.log("Trying to revert changes for '%s'" % simpleName)
        if openDocument(f):
            fileMenu = findObject("{name='QtCreator.Menu.File' title='File' type='QMenu'}")
            for menuItem in object.children(fileMenu):
                if str(menuItem.text) == 'Revert "%s" to Saved' % simpleName:
                    if (test.compare(canRevert, menuItem.enabled, "Verifying whether MenuItem "
                                     "'Revert to Saved' has expected state (%s)"
                                     % str(canRevert)) and canRevert):
                        invokeMenuItem('File', 'Revert "%s" to Saved' % simpleName)
                        clickButton(waitForObject(":Revert to Saved.Proceed_QPushButton"))
                        compareFileToOriginal(simpleName)
                        test.log("Reverted changes inside %s" % simpleName)
        else:
            test.fail("Could not open %s for reverting changes" % simpleName)

def compareFileToOriginal(fileName):
    global originalSources
    currentContent = str(waitForObject(getEditorForFileSuffix(fileName)).plainText)
    origFile = open(os.path.join(originalSources, fileName), "r")
    originalContent = origFile.read()
    origFile.close()
    test.compare(originalContent, currentContent,
                 "Comparing original to reverted file content for '%s'" % fileName)
