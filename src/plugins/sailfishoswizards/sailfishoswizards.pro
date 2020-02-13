include(../../qtcreatorplugin.pri)

DEFINES += SAILFISHOSWIZARDS_LIBRARY

INCLUDEPATH += include

SOURCES += \
    sailfishoswizardsplugin.cpp \
    src/pages/qmllocalimportspage.cpp \
    src/factories/qmllocalimportspagefactory.cpp

HEADERS += \
    sailfishoswizardsplugin.h \
    sailfishoswizards_global.h \
    sailfishoswizardsconstants.h \
    include/pages/qmllocalimportspage.h \
    include/factories/qmllocalimportspagefactory.h

FORMS += \
    forms/qmllocalimportspage.ui
