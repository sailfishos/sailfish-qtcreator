include(../../qtcreatorplugin.pri)

DEFINES += \
    NIM_LIBRARY

RESOURCES += \
    nim.qrc

INCLUDEPATH += $$PWD

HEADERS += \
    nimplugin.h \
    nimconstants.h \
    editor/nimcompletionassistprovider.h \
    editor/nimhighlighter.h \
    editor/nimindenter.h \
    editor/nimtexteditorwidget.h \
    project/nimblebuildconfiguration.h \
    project/nimblebuildstep.h \
    project/nimblebuildstepwidget.h \
    project/nimbleproject.h \
    project/nimblerunconfiguration.h \
    project/nimbletaskstep.h \
    project/nimbletaskstepwidget.h \
    tools/nimlexer.h \
    tools/sourcecodestream.h \
    project/nimbuildsystem.h \
    project/nimblebuildsystem.h \
    project/nimproject.h \
    project/nimprojectnode.h \
    project/nimbuildconfiguration.h \
    project/nimcompilerbuildstep.h \
    project/nimcompilerbuildstepconfigwidget.h \
    project/nimcompilercleanstep.h \
    project/nimrunconfiguration.h \
    editor/nimeditorfactory.h \
    settings/nimcodestylesettingspage.h \
    settings/nimcodestylepreferencesfactory.h \
    settings/nimsettings.h \
    settings/nimcodestylepreferenceswidget.h \
    settings/nimtoolssettingspage.h \
    project/nimtoolchain.h \
    project/nimtoolchainfactory.h \
    suggest/client.h \
    suggest/clientrequests.h \
    suggest/nimsuggest.h \
    suggest/nimsuggestcache.h \
    suggest/server.h \
    suggest/sexprlexer.h \
    suggest/sexprparser.h

SOURCES += \
    nimplugin.cpp \
    editor/nimcompletionassistprovider.cpp \
    editor/nimhighlighter.cpp \
    editor/nimindenter.cpp \
    editor/nimtexteditorwidget.cpp \
    project/nimblebuildconfiguration.cpp \
    project/nimblebuildstep.cpp \
    project/nimbletaskstep.cpp \
    project/nimblebuildstepwidget.cpp \
    project/nimbleproject.cpp \
    project/nimblerunconfiguration.cpp \
    project/nimbletaskstepwidget.cpp \
    tools/nimlexer.cpp \
    project/nimbuildsystem.cpp \
    project/nimblebuildsystem.cpp \
    project/nimproject.cpp \
    project/nimprojectnode.cpp \
    project/nimbuildconfiguration.cpp \
    project/nimcompilerbuildstep.cpp \
    project/nimcompilerbuildstepconfigwidget.cpp \
    project/nimcompilercleanstep.cpp \
    project/nimrunconfiguration.cpp \
    editor/nimeditorfactory.cpp \
    settings/nimcodestylesettingspage.cpp \
    settings/nimcodestylepreferencesfactory.cpp \
    settings/nimsettings.cpp \
    settings/nimcodestylepreferenceswidget.cpp \
    settings/nimtoolssettingspage.cpp \
    project/nimtoolchain.cpp \
    project/nimtoolchainfactory.cpp \
    suggest/client.cpp \
    suggest/clientrequests.cpp \
    suggest/nimsuggest.cpp \
    suggest/nimsuggestcache.cpp \
    suggest/server.cpp

FORMS += \
    project/nimblebuildstepwidget.ui \
    project/nimbletaskstepwidget.ui \
    project/nimcompilerbuildstepconfigwidget.ui \
    settings/nimcodestylepreferenceswidget.ui \
    settings/nimtoolssettingswidget.ui
