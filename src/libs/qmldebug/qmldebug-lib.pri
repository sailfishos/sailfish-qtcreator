contains(CONFIG, dll) {
    DEFINES += QMLDEBUG_LIB
} else {
    DEFINES += QMLDEBUG_STATIC_LIB
}

HEADERS += \
    $$PWD/qmldebugclient.h \
    $$PWD/baseenginedebugclient.h \
    $$PWD/declarativeenginedebugclient.h \
    $$PWD/declarativeenginedebugclientv2.h \
    $$PWD/qmloutputparser.h \
    $$PWD/qmldebug_global.h \
    $$PWD/qpacketprotocol.h \
    $$PWD/qmldebugconstants.h \
    $$PWD/qdebugmessageclient.h \
    $$PWD/qmlenginedebugclient.h \
    $$PWD/basetoolsclient.h \
    $$PWD/declarativetoolsclient.h \
    $$PWD/qmltoolsclient.h \
    $$PWD/qmlenginecontrolclient.h \
    $$PWD/qmldebugcommandlinearguments.h

SOURCES += \
    $$PWD/qmldebugclient.cpp \
    $$PWD/baseenginedebugclient.cpp \
    $$PWD/qmloutputparser.cpp \
    $$PWD/qpacketprotocol.cpp \
    $$PWD/qdebugmessageclient.cpp \
    $$PWD/basetoolsclient.cpp \
    $$PWD/declarativetoolsclient.cpp \
    $$PWD/qmltoolsclient.cpp \
    $$PWD/declarativeenginedebugclient.cpp \
    $$PWD/qmlenginecontrolclient.cpp

