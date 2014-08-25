QTC_LIB_DEPENDS += \
 ssh \
 utils

include(../../../qtcreator.pri)
include(../../rpath.pri)

CONFIG += console
CONFIG -= app_bundle

QT += network
QT -= gui


SOURCES = \
    main.cpp \
    command.cpp \
    qmakecommand.cpp \
    generatekeyscommand.cpp \
    gcccommand.cpp \
    merremoteprocess.cpp \
    makecommand.cpp \
    deploycommand.cpp \
    rpmcommand.cpp

HEADERS += \
    command.h \
    commandfactory.h \
    qmakecommand.h \
    generatekeyscommand.h \
    gcccommand.h \
    merremoteprocess.h \
    makecommand.h \
    deploycommand.h \
    rpmcommand.h

DESTDIR = $$IDE_LIBEXEC_PATH

target.path = $$QTC_PREFIX/bin # FIXME: libexec, more or less
INSTALLS += target
