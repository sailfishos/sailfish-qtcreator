import qbs 1.0

QtcPlugin {
    name: "Nim"

    Depends { name: "Qt.widgets" }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "TextEditor" }
    Depends { name: "ProjectExplorer" }

    Group {
        name: "General"
        files: [
            "nimplugin.cpp", "nimplugin.h",
            "nimconstants.h",
            "nim.qrc",
        ]
    }

    Group {
        name: "Editor"
        prefix: "editor/"
        files: [
            "nimeditorfactory.h", "nimeditorfactory.cpp",
            "nimhighlighter.h", "nimhighlighter.cpp",
            "nimindenter.h", "nimindenter.cpp"
        ]
    }

    Group {
        name: "Project"
        prefix: "project/"
        files: [
            "nimbuildconfiguration.h", "nimbuildconfiguration.cpp",
            "nimbuildconfigurationfactory.h", "nimbuildconfigurationfactory.cpp",
            "nimbuildconfigurationwidget.h", "nimbuildconfigurationwidget.cpp",
            "nimbuildinfo.h", "nimbuildinfo.cpp",
            "nimcompilerbuildstep.h", "nimcompilerbuildstep.cpp",
            "nimcompilerbuildstepconfigwidget.h", "nimcompilerbuildstepconfigwidget.cpp", "nimcompilerbuildstepconfigwidget.ui",
            "nimcompilerbuildstepfactory.h", "nimcompilerbuildstepfactory.cpp",
            "nimcompilercleanstep.h", "nimcompilercleanstep.cpp",
            "nimcompilercleanstepconfigwidget.h", "nimcompilercleanstepconfigwidget.cpp", "nimcompilercleanstepconfigwidget.ui",
            "nimcompilercleanstepfactory.h", "nimcompilercleanstepfactory.cpp",
            "nimproject.h", "nimproject.cpp",
            "nimprojectmanager.h", "nimprojectmanager.cpp",
            "nimprojectnode.h", "nimprojectnode.cpp",
            "nimrunconfiguration.h", "nimrunconfiguration.cpp",
            "nimrunconfigurationfactory.h", "nimrunconfigurationfactory.cpp",
            "nimrunconfigurationwidget.h", "nimrunconfigurationwidget.cpp",
            "nimruncontrol.h", "nimruncontrol.cpp",
            "nimruncontrolfactory.h", "nimruncontrolfactory.cpp"
        ]
    }

    Group {
        name: "Settings"
        prefix: "settings/"
        files: [
            "nimcodestylepreferencesfactory.h", "nimcodestylepreferencesfactory.cpp",
            "nimcodestylepreferenceswidget.h", "nimcodestylepreferenceswidget.cpp", "nimcodestylepreferenceswidget.ui",
            "nimcodestylesettingspage.h", "nimcodestylesettingspage.cpp",
            "nimsettings.h", "nimsettings.cpp",
            "nimsnippetprovider.h", "nimsnippetprovider.cpp",
        ]
    }

    Group {
        name: "Tools"
        prefix: "tools/"
        files: [
            "nimlexer.h", "nimlexer.cpp",
            "sourcecodestream.h"
        ]
    }
}
