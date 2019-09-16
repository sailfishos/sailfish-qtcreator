QTC_LIB_DEPENDS += \
 sfdk \
 ssh \
 utils

CONFIG += qtc_human_user_tool

include(../../qtcreatortool.pri)

QT += network xmlpatterns
QT -= gui

*-g++*:QMAKE_CXXFLAGS += -Wall -Werror

DEFINES += QT_NO_URL_CAST_FROM_STRING

HEADERS = \
    command.h \
    commandlineparser.h \
    configuration.h \
    dispatch.h \
    sdkmanager.h \
    remoteprocess.h \
    session.h \
    sfdkconstants.h \
    sfdkglobal.h \
    task.h \
    textutils.h \
    ../../plugins/mer/mersettings.h \

SOURCES = \
    command.cpp \
    commandlineparser.cpp \
    configuration.cpp \
    dispatch.cpp \
    main.cpp \
    remoteprocess.cpp \
    sdkmanager.cpp \
    session.cpp \
    sfdkglobal.cpp \
    task.cpp \
    textutils.cpp \
    ../../plugins/mer/mersettings.cpp \
