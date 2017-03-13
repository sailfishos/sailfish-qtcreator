import qbs 1.0

QtcPlugin {
    name: "BinEditor"

    Depends { name: "Qt.widgets" }
    Depends { name: "Aggregation" }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "TextEditor" }

    files: [
        "bineditorwidget.cpp",
        "bineditorwidget.h",
        "bineditorconstants.h",
        "bineditor_global.h",
        "bineditorplugin.cpp",
        "bineditorplugin.h",
        "markup.cpp",
        "markup.h",
    ]
}

