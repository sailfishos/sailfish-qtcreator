include(../../qtcreatorlibrary.pri)

DEFINES += SFDK_LIBRARY

QT += network

SOURCES += \
    sfdk.cpp \
    sfdkglobal.cpp \
    virtualboxmanager.cpp \
    vmconnection.cpp \

HEADERS += \
    sfdk.h \
    sfdkconstants.h \
    sfdkglobal.h \
    virtualboxmanager_p.h \
    vmconnection.h \
