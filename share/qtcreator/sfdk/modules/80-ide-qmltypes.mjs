Array.prototype.flatMap = function(fn) {
    return this.reduce((acc, i) => acc.concat(fn(i)), []);
}

export function formatOption(name, argument, needsArgument, defaultFormat) {
    var mb2Options = [
        "[no-]fix-version",
        "output-dir",
        "output-prefix",
        "no-pull-build-requires",
        "[no-]search-output-dir",
        "[no-]snapshot",
        "specfile",
        "[no-]task"
    ].flatMap(utils.expandCompacted);

    if (mb2Options.indexOf(name) >= 0)
        return [true, ["-Xmb2"].concat(defaultFormat)];

    return [false, []];
}
