include(../../qtcreatorlibrary.pri)

DEFINES += SFDK_LIBRARY

QT += network

SOURCES += \
    sfdk.cpp \
    sfdkglobal.cpp \
    vboxvirtualmachine.cpp \
    virtualboxmanager.cpp \
    virtualmachine.cpp \
    vmconnection.cpp \

HEADERS += \
    sfdk.h \
    sfdkconstants.h \
    sfdkglobal.h \
    vboxvirtualmachine_p.h \
    virtualboxmanager_p.h \
    virtualmachine.h \
    virtualmachine_p.h \
    vmconnection_p.h \
