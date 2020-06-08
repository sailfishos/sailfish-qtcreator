export function validateSpecFilePath(filePath) {
    if (utils.isFile(filePath))
        return [true, ""];

    if (utils.exists(filePath))
        return [false, qsTr("Not a file")];

    var yamlFilePath = filePath.replace(/\.spec/, ".yaml");
    if (utils.exists(yamlFilePath))
        return [true, ""];

    return [false, qsTr("No such file or related YAML template")]
}

export function validateBuildTargetName(name) {
    if (buildEngine.hasBuildTarget(name))
        return [true, ""];

    return [false, qsTr("No such build target")];
}

export function validateExistingDirectory(path) {
    if (utils.isDirectory(path))
        return [true, ""];

    if (utils.exists(path))
        return [false, qsTr("Not a directory")];

    return [false, qsTr("Directory does not exist")];
}

export function validateSearchOutputDirOption(value) {
    if (value === "verbose" || value === "quiet")
        return [true, ""];

    return [false, qsTr("Invalid keyword used")];
}

export function mapCompilationDatabasePaths() {
    if (!utils.isFile("compile_commands.json"))
        return;
    utils.updateFile("compile_commands.json", function (data) {
        var sharedHomeMountPointRx =
            new RegExp(utils.regExpEscape(buildEngine.sharedHomeMountPoint), "g");
        data = data.replace(sharedHomeMountPointRx, buildEngine.sharedHomePath);
        var sharedSrcMountPointRx =
            new RegExp(utils.regExpEscape(buildEngine.sharedSrcMountPoint), "g");
        data = data.replace(sharedSrcMountPointRx, buildEngine.sharedSrcPath);
        var sysroot = buildEngine.sharedTargetsPath + '/' + configuration.optionArgument('target');
        data = data.replace(/("[^/]*)\/(usr|lib|opt)\b/g, "$1" + sysroot + "/$2");
        return data;
    });
}