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


export function maybeImportSigningKey() {
    if (!configuration.isOptionSet("package.signing-user"))
        return [true, ""];

    var signingUser = configuration.optionArgument("package.signing-user");
    var signingPassphraseFile = configuration.isOptionSet("package.signing-passphrase-file")
        ? configuration.optionArgument("package.signing-passphrase-file")
        : "";
    if (!buildEngine.importGpgKey(signingUser, signingPassphraseFile))
        return [false, qsTr("Failed to share GnuPG key with the build engine")];

    return [true, ""];
}
