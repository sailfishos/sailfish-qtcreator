QTC_LIB_DEPENDS += \
    utils

include(../../qtcreatortool.pri)

QT -= gui test

QT += xmlpatterns

isEmpty(PRECOMPILED_HEADER):PRECOMPILED_HEADER = $$PWD/../../shared/qtcreator_pch.h

SOURCES += \
    main.cpp \
    adddebuggeroperation.cpp \
    adddeviceoperation.cpp \
    addkeysoperation.cpp \
    addkitoperation.cpp \
    addqtoperation.cpp \
    addtoolchainoperation.cpp \
    findkeyoperation.cpp \
    findvalueoperation.cpp \
    getoperation.cpp \
    operation.cpp \
    rmdebuggeroperation.cpp \
    rmdeviceoperation.cpp \
    rmkeysoperation.cpp \
    rmkitoperation.cpp \
    rmqtoperation.cpp \
    rmtoolchainoperation.cpp \
    settings.cpp \
    addmertargetoperation.cpp \
    rmmertargetoperation.cpp \
    addmersdkoperation.cpp \
    ../../plugins/mer/mertargetsxmlparser.cpp \
    rmmersdkoperation.cpp \

HEADERS += \
    adddebuggeroperation.h \
    adddeviceoperation.h \
    addkeysoperation.h \
    addkitoperation.h \
    addqtoperation.h \
    addtoolchainoperation.h \
    findkeyoperation.h \
    findvalueoperation.h \
    getoperation.h \
    operation.h \
    rmdebuggeroperation.h \
    rmdeviceoperation.h \
    rmkeysoperation.h \
    rmkitoperation.h \
    rmqtoperation.h \
    rmtoolchainoperation.h \
    settings.h \
    addmertargetoperation.h \
    rmmertargetoperation.h \
    addmersdkoperation.h \
    ../../plugins/mer/mertargetsxmlparser.h \
    rmmersdkoperation.h \

macx:DEFINES += "DATA_PATH=\"\\\".\\\"\""
else:DEFINES += "DATA_PATH=\"\\\"../share/qtcreator\\\"\""
