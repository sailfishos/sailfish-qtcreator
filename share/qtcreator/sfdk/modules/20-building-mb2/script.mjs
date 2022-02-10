export * from '../10-general/script.mjs'

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

    try {
        buildEngine.importGpgKey(signingUser, signingPassphraseFile)
    } catch (e) {
        return [false, qsTr("Failed to share GnuPG key with the build engine: %1").arg(e.message)];
    }

    return [true, ""];
}

export function filterCMakeCommandLine(args) {
    if (args.lenth === 0 || args[0] === "--build")
        return args;

    // See MerSdkManager::ensureCmakeToolIsSet() and Sfdk::CMakeHelper::doCMakeApiCacheReplyPathMapping()
    return args.filter(arg =>
        !arg.startsWith("-DCMAKE_CXX_COMPILER:")
        && !arg.startsWith("-DCMAKE_C_COMPILER:")
        && !arg.startsWith("-DCMAKE_PREFIX_PATH:")
        && !arg.startsWith("-DCMAKE_SYSROOT:")
        && !arg.startsWith("-DQT_QMAKE_EXECUTABLE:")
    );
}

export function filterBuildCommandLine(args) {
    return args.concat("--no-rpmlint");
}
