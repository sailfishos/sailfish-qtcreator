DEFINES += TEXTEDITOR_LIBRARY
QT += network printsupport xml
CONFIG += exceptions
CONFIG += include_source_dir # For the highlighter autotest.
include(../../qtcreatorplugin.pri)
SOURCES += texteditorplugin.cpp \
    plaintexteditorfactory.cpp \
    textdocument.cpp \
    texteditor.cpp \
    behaviorsettings.cpp \
    behaviorsettingspage.cpp \
    texteditoractionhandler.cpp \
    fontsettingspage.cpp \
    texteditorconstants.cpp \
    tabsettings.cpp \
    storagesettings.cpp \
    displaysettings.cpp \
    displaysettingspage.cpp \
    fontsettings.cpp \
    linenumberfilter.cpp \
    findinfiles.cpp \
    basefilefind.cpp \
    texteditorsettings.cpp \
    codecselector.cpp \
    findincurrentfile.cpp \
    findinopenfiles.cpp \
    colorscheme.cpp \
    colorschemeedit.cpp \
    texteditoroverlay.cpp \
    texteditoroptionspage.cpp \
    textdocumentlayout.cpp \
    completionsettings.cpp \
    normalindenter.cpp \
    indenter.cpp \
    quickfix.cpp \
    syntaxhighlighter.cpp \
    highlighterutils.cpp \
    generichighlighter/itemdata.cpp \
    generichighlighter/specificrules.cpp \
    generichighlighter/rule.cpp \
    generichighlighter/dynamicrule.cpp \
    generichighlighter/context.cpp \
    generichighlighter/includerulesinstruction.cpp \
    generichighlighter/progressdata.cpp \
    generichighlighter/keywordlist.cpp \
    generichighlighter/highlightdefinition.cpp \
    generichighlighter/highlighter.cpp \
    generichighlighter/manager.cpp \
    generichighlighter/highlightdefinitionhandler.cpp \
    generichighlighter/highlightersettingspage.cpp \
    generichighlighter/highlightersettings.cpp \
    generichighlighter/managedefinitionsdialog.cpp \
    generichighlighter/definitiondownloader.cpp \
    refactoringchanges.cpp \
    refactoroverlay.cpp \
    outlinefactory.cpp \
    basehoverhandler.cpp \
    helpitem.cpp \
    autocompleter.cpp \
    snippets/snippetssettingspage.cpp \
    snippets/snippet.cpp \
    snippets/snippeteditor.cpp \
    snippets/snippetscollection.cpp \
    snippets/snippetssettings.cpp \
    snippets/isnippetprovider.cpp \
    snippets/plaintextsnippetprovider.cpp \
    behaviorsettingswidget.cpp \
    extraencodingsettings.cpp \
    codeassist/functionhintproposalwidget.cpp \
    codeassist/ifunctionhintproposalmodel.cpp \
    codeassist/functionhintproposal.cpp \
    codeassist/iassistprovider.cpp \
    codeassist/iassistproposal.cpp \
    codeassist/iassistprocessor.cpp \
    codeassist/iassistproposalwidget.cpp \
    codeassist/codeassistant.cpp \
    snippets/snippetassistcollector.cpp \
    codeassist/assistinterface.cpp \
    codeassist/assistproposalitem.cpp \
    convenience.cpp \
    codeassist/runner.cpp \
    codeassist/completionassistprovider.cpp \
    codeassist/genericproposalmodel.cpp \
    codeassist/quickfixassistprovider.cpp \
    codeassist/quickfixassistprocessor.cpp \
    codeassist/genericproposal.cpp \
    codeassist/genericproposalwidget.cpp \
    codeassist/iassistproposalmodel.cpp \
    tabsettingswidget.cpp \
    simplecodestylepreferences.cpp \
    simplecodestylepreferenceswidget.cpp \
    icodestylepreferencesfactory.cpp \
    semantichighlighter.cpp \
    codestyleselectorwidget.cpp \
    typingsettings.cpp \
    icodestylepreferences.cpp \
    codestylepool.cpp \
    codestyleeditor.cpp \
    circularclipboard.cpp \
    circularclipboardassist.cpp \
    textmark.cpp \
    codeassist/keywordscompletionassist.cpp \
    marginsettings.cpp

HEADERS += texteditorplugin.h \
    plaintexteditorfactory.h \
    texteditor_p.h \
    textdocument.h \
    behaviorsettings.h \
    behaviorsettingspage.h \
    texteditor.h \
    texteditoractionhandler.h \
    fontsettingspage.h \
    texteditorconstants.h \
    tabsettings.h \
    storagesettings.h \
    displaysettings.h \
    displaysettingspage.h \
    fontsettings.h \
    linenumberfilter.h \
    texteditor_global.h \
    findinfiles.h \
    basefilefind.h \
    texteditorsettings.h \
    codecselector.h \
    findincurrentfile.h \
    findinopenfiles.h \
    colorscheme.h \
    colorschemeedit.h \
    texteditoroverlay.h \
    texteditoroptionspage.h \
    textdocumentlayout.h \
    completionsettings.h \
    normalindenter.h \
    indenter.h \
    quickfix.h \
    syntaxhighlighter.h \
    highlighterutils.h \
    generichighlighter/reuse.h \
    generichighlighter/itemdata.h \
    generichighlighter/specificrules.h \
    generichighlighter/rule.h \
    generichighlighter/reuse.h \
    generichighlighter/dynamicrule.h \
    generichighlighter/context.h \
    generichighlighter/includerulesinstruction.h \
    generichighlighter/progressdata.h \
    generichighlighter/keywordlist.h \
    generichighlighter/highlighterexception.h \
    generichighlighter/highlightdefinition.h \
    generichighlighter/highlighter.h \
    generichighlighter/manager.h \
    generichighlighter/highlightdefinitionhandler.h \
    generichighlighter/highlightersettingspage.h \
    generichighlighter/highlightersettings.h \
    generichighlighter/managedefinitionsdialog.h \
    generichighlighter/highlightdefinitionmetadata.h \
    generichighlighter/definitiondownloader.h \
    refactoringchanges.h \
    refactoroverlay.h \
    outlinefactory.h \
    ioutlinewidget.h \
    basehoverhandler.h \
    helpitem.h \
    autocompleter.h \
    snippets/snippetssettingspage.h \
    snippets/snippet.h \
    snippets/snippeteditor.h \
    snippets/snippetscollection.h \
    snippets/reuse.h \
    snippets/snippetssettings.h \
    snippets/isnippetprovider.h \
    snippets/plaintextsnippetprovider.h \
    behaviorsettingswidget.h \
    extraencodingsettings.h \
    codeassist/functionhintproposalwidget.h \
    codeassist/ifunctionhintproposalmodel.h \
    codeassist/functionhintproposal.h \
    codeassist/iassistprovider.h \
    codeassist/iassistprocessor.h \
    codeassist/iassistproposalwidget.h \
    codeassist/iassistproposal.h \
    codeassist/codeassistant.h \
    snippets/snippetassistcollector.h \
    codeassist/assistinterface.h \
    codeassist/assistproposalitem.h \
    convenience.h \
    codeassist/assistenums.h \
    codeassist/runner.h \
    codeassist/completionassistprovider.h \
    codeassist/genericproposalmodel.h \
    codeassist/quickfixassistprovider.h \
    codeassist/quickfixassistprocessor.h \
    codeassist/genericproposal.h \
    codeassist/genericproposalwidget.h \
    codeassist/iassistproposalmodel.h \
    tabsettingswidget.h \
    simplecodestylepreferences.h \
    simplecodestylepreferenceswidget.h \
    icodestylepreferencesfactory.h \
    semantichighlighter.h \
    codestyleselectorwidget.h \
    typingsettings.h \
    icodestylepreferences.h \
    codestylepool.h \
    codestyleeditor.h \
    basefilefind_p.h \
    circularclipboard.h \
    circularclipboardassist.h \
    textmark.h \
    codeassist/keywordscompletionassist.h \
    textmarkregistry.h \
    marginsettings.h

FORMS += \
    displaysettingspage.ui \
    fontsettingspage.ui \
    colorschemeedit.ui \
    generichighlighter/highlightersettingspage.ui \
    generichighlighter/managedefinitionsdialog.ui \
    snippets/snippetssettingspage.ui \
    behaviorsettingswidget.ui \
    behaviorsettingspage.ui \
    tabsettingswidget.ui \
    codestyleselectorwidget.ui
RESOURCES += texteditor.qrc

equals(TEST, 1) {
SOURCES += texteditor_test.cpp
}

