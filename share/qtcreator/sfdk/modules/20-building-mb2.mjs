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
    // Projects may use a subdirectory to store sources
    var maxDepth = 1;
    // Do a wide search as multiple compile DBs may appear deeper in the tree
    // depending on the way a particular package is built.
    var compilationDb = utils.findFile_wide(".", maxDepth, "compile_commands.json");
    if (!compilationDb)
        return;
    utils.updateFile(compilationDb, function (data) {
        var target = configuration.optionArgument('target');
        var sysroot = buildEngine.sharedTargetsPath + '/' + target;
        var toolsPath = buildEngine.buildTargetToolsPath(target);

        var objects = JSON.parse(data);

        var badCommands = new Set;
        objects = objects.reduce((acc, object) => {
            var command = object.arguments[0];
            if (/\/(.*-)?(gcc|g\+\+|c\+\+|cc|c89|c99)$/.test(command)) {
                command = toolsPath + "/gcc";
            } else {
                if (!badCommands.has(command)) {
                    console.warn(qsTr("%1: Unrecognized compiler executable \"%2\" - unit(s) excluded")
                            .arg(compilationDb).arg(command));
                    badCommands.add(command);
                }
                return acc;
            }
            object.arguments[0] = command;

            acc.push(object);
            return acc;
        }, []);

        data = JSON.stringify(objects, null, 1);

        var sharedHomeMountPointRx =
            new RegExp(utils.regExpEscape(buildEngine.sharedHomeMountPoint), "g");
        data = data.replace(sharedHomeMountPointRx, buildEngine.sharedHomePath);
        var sharedSrcMountPointRx =
            new RegExp(utils.regExpEscape(buildEngine.sharedSrcMountPoint), "g");
        data = data.replace(sharedSrcMountPointRx, buildEngine.sharedSrcPath);

        data = data.replace(/("[^/]*)\/(usr|lib|opt)\b/g, "$1" + sysroot + "/$2");

        return data;
    });
}
