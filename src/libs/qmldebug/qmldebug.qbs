import qbs 1.0

Project {
    name: "QmlDebug"

    QtcLibrary {
        cpp.defines: base.concat("QMLDEBUG_LIBRARY")

        Depends { name: "Qt"; submodules: ["gui", "network"] }
        Depends { name: "Utils" }

        files: [
            "baseenginedebugclient.cpp",
            "baseenginedebugclient.h",
            "basetoolsclient.cpp",
            "basetoolsclient.h",
            "qdebugmessageclient.cpp",
            "qdebugmessageclient.h",
            "qmldebug_global.h",
            "qmldebugclient.cpp",
            "qmldebugclient.h",
            "qmldebugcommandlinearguments.h",
            "qmldebugconnection.cpp",
            "qmldebugconnection.h",
            "qmldebugconnectionmanager.cpp",
            "qmldebugconnectionmanager.h",
            "qmldebugconstants.h",
            "qmlenginecontrolclient.cpp",
            "qmlenginecontrolclient.h",
            "qmlenginedebugclient.h",
            "qmloutputparser.cpp",
            "qmloutputparser.h",
            "qmltoolsclient.cpp",
            "qmltoolsclient.h",
            "qpacketprotocol.cpp",
            "qpacketprotocol.h",
        ]
    }
}
