include(../../qtcreatorplugin.pri)

DEFINES += SAILFISHOSWIZARDS_LIBRARY

INCLUDEPATH += include

SOURCES += \
    sailfishoswizardsplugin.cpp \
    src/pages/qmllocalimportspage.cpp \
    src/factories/qmllocalimportspagefactory.cpp \
    src/factories/importexternallibrariespagefactory.cpp \
    src/pages/importexternallibrariespage.cpp \
    src/models/externallibrarylistmodel.cpp \
    src/models/externallibrary.cpp \
    utils.cpp

HEADERS += \
    sailfishoswizardsplugin.h \
    sailfishoswizards_global.h \
    sailfishoswizardsconstants.h \
    include/pages/qmllocalimportspage.h \
    include/factories/qmllocalimportspagefactory.h \
    include/pages/importexternallibrariespage.h \
    include/factories/importexternallibrariespagefactory.h \
    include/models/externallibrarylistmodel.h \
    include/models/externallibrary.h \
    utils.h

FORMS += \
    forms/qmllocalimportspage.ui \
    forms/importexternallibrariespage.ui \
    forms/selectitemsdialog.ui

RESOURCES += \
    resources.qrc
