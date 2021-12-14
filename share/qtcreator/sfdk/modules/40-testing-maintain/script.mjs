export function validateDeviceName(name) {
    if (utils.isDevice(name))
        return [true, ""];

    return [false, qsTr("No such device")];
}
