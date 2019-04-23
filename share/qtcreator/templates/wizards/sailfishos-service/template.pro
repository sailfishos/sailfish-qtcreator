TARGET = %{ProjectName}
TEMPLATE = app

QT = core
@if '%{ServiceType}' === 'dbus'
QT += dbus
@endif

CONFIG += link_pkgconfig

TARGETPATH = /usr/bin
target.path = $$TARGETPATH
INSTALLS += target

autostart.files = systemd/%{ServiceName}.service
autostart.path = /usr/lib/systemd/user
INSTALLS += autostart    

@if '%{ServiceType}' === 'dbus'
dbus.files = dbus/%{NamePrefix}.%{CompanyName}.%{ServiceName}.service
dbus.path = /usr/share/dbus-1/services/
INSTALLS += dbus

@endif
SOURCES += \\
    src/main.cpp \\
    src/%{SrcFileName}

HEADERS += \\
    src/%{HdrFileName}

OTHER_FILES += rpm/$${TARGET}.spec
