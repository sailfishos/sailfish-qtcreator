export function validateExistingDirectory(path) {
    if (utils.isDirectory(path))
        return [true, ""];

    if (utils.exists(path))
        return [false, qsTr("Not a directory")];

    return [false, qsTr("Directory does not exist")];
}

