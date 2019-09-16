include(../../qtcreatorlibrary.pri)

DEFINES += SFDK_LIBRARY
DEFINES += QT_NO_URL_CAST_FROM_STRING

QT += network xmlpatterns

SOURCES += \
    asynchronous.cpp \
    buildengine.cpp \
    sfdkglobal.cpp \
    sdk.cpp \
    targetsxmlreader.cpp \
    usersettings.cpp \
    vboxvirtualmachine.cpp \
    virtualmachine.cpp \
    vmconnection.cpp \

HEADERS += \
    asynchronous.h \
    asynchronous_p.h \
    buildengine.h \
    buildengine_p.h \
    sfdkconstants.h \
    sfdkglobal.h \
    sdk.h \
    sdk_p.h \
    targetsxmlreader_p.h \
    usersettings_p.h \
    vboxvirtualmachine_p.h \
    virtualmachine.h \
    virtualmachine_p.h \
    vmconnection_p.h \
