import Nemo.DBus 2.0

DBusInterface {
    id: dbusService

    service: '%{serviceName}'
    path: '%{servicePath}'
    iface: '%{interfaceName}'
    bus: DBus.%{busType}Bus%{signalsMode}%{methods}
}%{signals}
