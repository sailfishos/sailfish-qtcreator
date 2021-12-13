function validateList(string) {
    return string.split(",").every(item => /^[-+]?\w[-.\w]*$/.test(item));
}

export function validateCheckLevels(argument) {
    if (!validateList(argument))
        return [false, qsTr("Syntax error")];

    var levels = argument.split(",").map(l => l.replace(/^[-+]/, ""));
    var known = ["static", "source", "package", "dynamic", "unit", "system"];

    var unknown = levels.filter(l => !known.includes(l));
    if (unknown.length != 0)
        return [false, qsTr("Not a valid level name: \"%1\"").arg(unknown[0])];

    return [true, ""];
}

export function validateCheckSuites(argument) {
    if (!validateList(argument))
        return [false, qsTr("Syntax error")];

    return [true, ""];
}
