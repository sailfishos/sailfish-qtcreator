include(../../qtcreatorlibrary.pri)

DEFINES += SFDK_LIBRARY

QT += network

SOURCES += \
    sfdkglobal.cpp \
    sdk.cpp \
    vboxvirtualmachine.cpp \
    virtualboxmanager.cpp \
    virtualmachine.cpp \
    vmconnection.cpp \

HEADERS += \
    sfdkconstants.h \
    sfdkglobal.h \
    sdk.h \
    sdk_p.h \
    vboxvirtualmachine_p.h \
    virtualboxmanager_p.h \
    virtualmachine.h \
    virtualmachine_p.h \
    vmconnection_p.h \
