include(../../qtcreatorlibrary.pri)

DEFINES += SFDK_LIBRARY
DEFINES += QT_NO_URL_CAST_FROM_STRING

QT += network xmlpatterns

*-g++*:QMAKE_CXXFLAGS += -Wall -Werror

SOURCES += \
    asynchronous.cpp \
    buildengine.cpp \
    device.cpp \
    emulator.cpp \
    sfdkglobal.cpp \
    sdk.cpp \
    targetsxmlreader.cpp \
    utils.cpp \
    usersettings.cpp \
    vboxvirtualmachine.cpp \
    virtualmachine.cpp \
    vmconnection.cpp \

HEADERS += \
    asynchronous.h \
    asynchronous_p.h \
    buildengine.h \
    buildengine_p.h \
    device.h \
    device_p.h \
    emulator.h \
    emulator_p.h \
    sfdkconstants.h \
    sfdkglobal.h \
    sdk.h \
    sdk_p.h \
    targetsxmlreader_p.h \
    utils_p.h \
    usersettings_p.h \
    vboxvirtualmachine_p.h \
    virtualmachine.h \
    virtualmachine_p.h \
    vmconnection_p.h \
