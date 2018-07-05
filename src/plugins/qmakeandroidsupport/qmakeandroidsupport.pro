QT += network
include(../../qtcreatorplugin.pri)

DEFINES += \
    QMAKEANDROID_LIBRARY

HEADERS += \
    androidextralibrarylistmodel.h \
    createandroidmanifestwizard.h \
    qmakeandroidsupport.h \
    qmakeandroidrunconfiguration.h \
    qmakeandroidrunfactories.h \
    qmakeandroidbuildapkstep.h \
    qmakeandroidbuildapkwidget.h \
    androidqmakebuildconfigurationfactory.h \
    qmakeandroidsupportplugin.h

SOURCES += \
    androidextralibrarylistmodel.cpp \
    createandroidmanifestwizard.cpp \
    qmakeandroidsupport.cpp \
    qmakeandroidrunconfiguration.cpp \
    qmakeandroidrunfactories.cpp \
    qmakeandroidbuildapkstep.cpp \
    qmakeandroidbuildapkwidget.cpp \
    androidqmakebuildconfigurationfactory.cpp \
    qmakeandroidsupportplugin.cpp

FORMS += qmakeandroidbuildapkwidget.ui
