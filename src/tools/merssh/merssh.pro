QTC_LIB_DEPENDS += \
 ssh \
 utils

include(../../qtcreatortool.pri)

# See qtcreator.pri
RELATIVE_REVERSE_LIBEXEC_PATH = $$relative_path($$IDE_BIN_PATH, $$IDE_LIBEXEC_PATH)
DEFINES += $$shell_quote(RELATIVE_REVERSE_LIBEXEC_PATH=\"$$RELATIVE_REVERSE_LIBEXEC_PATH\")

QT += network
QT -= gui

SOURCES = \
    main.cpp \
    command.cpp \
    cmakecommand.cpp \
    qmakecommand.cpp \
    generatekeyscommand.cpp \
    gcccommand.cpp \
    makecommand.cpp \
    deploycommand.cpp \
    rpmcommand.cpp \
    rpmvalidationcommand.cpp \
    lupdatecommand.cpp \

HEADERS += \
    command.h \
    commandfactory.h \
    cmakecommand.h \
    qmakecommand.h \
    generatekeyscommand.h \
    gcccommand.h \
    makecommand.h \
    deploycommand.h \
    rpmcommand.h \
    rpmvalidationcommand.h \
    lupdatecommand.h \
