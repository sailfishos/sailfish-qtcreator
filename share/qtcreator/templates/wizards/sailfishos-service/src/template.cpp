#include "%{HdrFileName}"
#include <QCoreApplication>
@if '%{ServiceType}' === 'dbus'
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDebug>
@endif

%{ClassName}::%{ClassName}(QObject *parent) : QObject(parent)
{

@if '%{ServiceType}' === 'dbus'
    const QString serviceName = QStringLiteral("%{NamePrefix}.%{CompanyName}.%{ServiceName}");

    if (QDBusConnection::sessionBus().interface()->isServiceRegistered(serviceName)) {
        qWarning() << Q_FUNC_INFO << "Service already registered!";
        qApp->quit();
        return;
    }

    bool success = false;
    success = QDBusConnection::sessionBus().registerObject(QStringLiteral("/%{NamePrefix}/%{CompanyName}/%{ServiceName}"), this, QDBusConnection::ExportScriptableSlots);
    if (!success) {
        qWarning () << Q_FUNC_INFO << "Failed to register object!";
        qApp->quit();
        return;
    }

    success = QDBusConnection::sessionBus().registerService(serviceName);
    if (!success) {
        qWarning () << Q_FUNC_INFO << "Failed to register service!";
        qApp->quit();
    }
@endif

}

void %{ClassName}::start()
{

}
