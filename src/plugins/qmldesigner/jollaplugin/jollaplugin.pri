TARGET = jollaplugin
TEMPLATE = lib
CONFIG += plugin

include (../designercore/iwidgetplugin.pri)

SOURCES += $$PWD/jollaplugin.cpp

HEADERS += $$PWD/jollaplugin.h $$PWD/../designercore/include/iwidgetplugin.h

RESOURCES += $$PWD/jollaplugin.qrc

OTHER_FILES += $$PWD/jolla.metainfo
