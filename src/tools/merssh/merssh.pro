QTC_LIB_DEPENDS += \
 ssh \
 utils

include(../../qtcreatortool.pri)

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
    rpmcommand.cpp \
    rpmvalidationcommand.cpp \
    lupdatecommand.cpp \
    wwwproxycommand.cpp \

HEADERS += \
    command.h \
    commandfactory.h \
    qmakecommand.h \
    generatekeyscommand.h \
    gcccommand.h \
    merremoteprocess.h \
    makecommand.h \
    deploycommand.h \
    rpmcommand.h \
    rpmvalidationcommand.h \
    lupdatecommand.h \
    wwwproxycommand.h \
