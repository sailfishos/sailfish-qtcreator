import qbs.base 1.0
import "../QtcTool.qbs" as QtcTool

QtcTool {
    name: "merssh"

    Depends { name: "Qt"; submodules: ["core", "network"] }
    Depends { name: "QtcSsh" }
    Depends { name: "app_version_header" }

    files: [
        "main.cpp",
        "merssh.h",
        "merssh.h"
    ]
}
