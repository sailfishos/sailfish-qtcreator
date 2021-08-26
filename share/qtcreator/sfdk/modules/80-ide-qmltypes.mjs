Array.prototype.flatMap = function(fn) {
    return this.reduce((acc, i) => acc.concat(fn(i)), []);
}

export function formatOption(name, argument, needsArgument, defaultFormat) {
    var mb2Options = [
        "output-dir",
        "output-prefix",
        "[no-]snapshot",
        "specfile",
        "task"
    ].flatMap(utils.expandCompacted);

    if (mb2Options.indexOf(name) >= 0)
        return [true, ["-Xmb2"].concat(defaultFormat)];

    return [false, []];
}
