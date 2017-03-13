import qbs 1.0

QtcTool {
    name: "iostool"
    condition: qbs.targetOS.contains("macos")

    Depends { name: "bundle" }
    Depends { name: "Qt.widgets" }
    Depends { name: "Qt.xml" }
    Depends { name: "Qt.network" }
    Depends { name: "app_version_header" }

    files: [
        "main.cpp",
        "iosdevicemanager.cpp",
        "iosdevicemanager.h"
    ]
    cpp.frameworks: base.concat(["CoreFoundation", "CoreServices", "IOKit", "Security",
                                 "SystemConfiguration"])
    bundle.infoPlistFile: "Info.plist"

    installDir: qtc.ide_libexec_path + "/ios"
}
