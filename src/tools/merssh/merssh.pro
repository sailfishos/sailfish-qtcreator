QTC_LIB_DEPENDS += \
 ssh \
 utils

include(../../qtcreatortool.pri)

QT += network
QT -= gui

SOURCES = \
    main.cpp \
    command.cpp \
    enginectlcommand.cpp \
    qmakecommand.cpp \
    generatekeyscommand.cpp \
    gcccommand.cpp \
    merremoteprocess.cpp \
    makecommand.cpp \
    deploycommand.cpp \
    rpmcommand.cpp \
    rpmvalidationcommand.cpp \
    lupdatecommand.cpp \
    ../../plugins/mer/merconnection.cpp \
    ../../plugins/mer/merlogging.cpp \
    ../../plugins/mer/mervirtualboxmanager.cpp

HEADERS += \
    command.h \
    commandfactory.h \
    enginectlcommand.h \
    qmakecommand.h \
    generatekeyscommand.h \
    gcccommand.h \
    merremoteprocess.h \
    makecommand.h \
    deploycommand.h \
    rpmcommand.h \
    rpmvalidationcommand.h \
    lupdatecommand.h \
    ../../plugins/mer/merconnection.h \
    ../../plugins/mer/merlogging.h \
    ../../plugins/mer/mervirtualboxmanager.h
