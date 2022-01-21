export function validateExistingWorkspacePath(path) {
    if (!utils.exists(path))
        return [false, qsTr("No such file or directory")];

    var cleanPath = utils.canonicalPath(utils.cleanPath(path));
    if (cleanPath != buildEngine.sharedSrcPath
            && !cleanPath.startsWith(buildEngine.sharedSrcPath + utils.pathSeparator)) {
        return [false, qsTr("Path not located under %1 workspace directory (%2)")
            .arg(utils.sdkVariant)
            .arg(buildEngine.sharedSrcPath)];
    }

    // See how SdkManager::mapEnginePaths works
    if (utils.isAbsolute(path) && path != cleanPath)
        return [false, qsTr("Not a clean path. Check for symbolic links and redundant \".\" or \"..\" components.")];

    return [true, ""];
}

export function validateExistingWorkspaceDirectory(path) {
    if (utils.exists(path) && !utils.isDirectory(path))
        return [false, qsTr("Not a directory")];

    return validateExistingWorkspacePath(path);
}

export function validateExistingWorkspaceFile(path) {
    if (utils.exists(path) && !utils.isFile(path))
        return [false, qsTr("Not a file")];

    return validateExistingWorkspacePath(path);
}
