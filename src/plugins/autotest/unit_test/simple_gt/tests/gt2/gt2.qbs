import qbs
import qbs.File
import qbs.FileInfo
import "../common/functions.js" as googleCommon

CppApplication {
    type: "application"
    name: "googletest2"

    property string gtestDir: googleCommon.getGTestDir(qbs, project.googletestDir)
    property string gmockDir: googleCommon.getGMockDir(qbs, project.googletestDir)

    Depends { name: "Qt.core" }

    condition: {
        if (File.exists(gtestDir) && File.exists(gmockDir))
            return true;

        console.error("Cannot find Google Test - specify environment variable GOOGLETEST_DIR "
                      + "or qbs property " + project.name + ".googletestDir" );
        return false;
    }

    cpp.includePaths: [].concat(googleCommon.getGTestIncludes(qbs, project.googletestDir))
                        .concat(googleCommon.getGMockIncludes(qbs, project.googletestDir))

    cpp.cxxLanguageVersion: "c++11"
    cpp.defines: ["GTEST_LANG_CXX11"]

    files: [
        // own stuff
        "queuetest.h",
        "main.cpp",
    ].concat(googleCommon.getGTestAll(qbs, project.googletestDir))
     .concat(googleCommon.getGMockAll(qbs, project.googletestDir))
}
