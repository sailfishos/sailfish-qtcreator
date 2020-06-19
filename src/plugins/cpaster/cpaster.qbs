import qbs 1.0

QtcPlugin {
    name: "CodePaster"

    Depends { name: "Qt"; submodules: ["widgets", "network"] }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "TextEditor" }

    cpp.includePaths: base.concat([project.sharedSourcesDir + "/cpaster"])
    cpp.defines: ["CPASTER_PLUGIN_GUI"]

    files: [
        "columnindicatortextedit.cpp",
        "columnindicatortextedit.h",
        "cpasterconstants.h",
        "cpaster.qrc",
        "cpasterplugin.cpp",
        "cpasterplugin.h",
        "fileshareprotocol.cpp",
        "fileshareprotocol.h",
        "fileshareprotocolsettingspage.cpp",
        "fileshareprotocolsettingspage.h",
        "fileshareprotocolsettingswidget.ui",
        "pastebindotcomprotocol.cpp",
        "pastebindotcomprotocol.h",
        "pastebindotcomsettings.ui",
        "pastecodedotxyzprotocol.cpp",
        "pastecodedotxyzprotocol.h",
        "pasteselect.ui",
        "pasteselectdialog.cpp",
        "pasteselectdialog.h",
        "pasteview.cpp",
        "pasteview.h",
        "pasteview.ui",
        "protocol.cpp",
        "protocol.h",
        "settings.cpp",
        "settings.h",
        "settingspage.cpp",
        "settingspage.h",
        "settingspage.ui",
        "stickynotespasteprotocol.cpp",
        "stickynotespasteprotocol.h",
        "urlopenprotocol.cpp",
        "urlopenprotocol.h",
    ]

    Group {
        name: "Shared"
        prefix: "../../shared/cpaster/"
        files: [
            "cgi.cpp",
            "cgi.h",
            "splitter.cpp",
            "splitter.h",
        ]
    }
}
