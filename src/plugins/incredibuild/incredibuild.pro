TARGET = IncrediBuild
TEMPLATE = lib

include(../../qtcreatorplugin.pri)

DEFINES += INCREDIBUILD_LIBRARY

# IncrediBuild files

SOURCES += incredibuildplugin.cpp \
    buildconsolebuildstep.cpp \
    commandbuilder.cpp \
    commandbuilderaspect.cpp \
    makecommandbuilder.cpp \
    cmakecommandbuilder.cpp \
    ibconsolebuildstep.cpp

HEADERS += incredibuildplugin.h \
    cmakecommandbuilder.h \
    commandbuilder.h \
    commandbuilderaspect.h \
    incredibuild_global.h \
    incredibuildconstants.h \
    buildconsolebuildstep.h \
    makecommandbuilder.h \
    ibconsolebuildstep.h \
