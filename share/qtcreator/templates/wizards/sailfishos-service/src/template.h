#ifndef QAUSERSERVICE_H
#define QAUSERSERVICE_H

#include <QObject>

class %{ClassName} : public QObject
{
    Q_OBJECT
    @if '%{ServiceType}' === 'dbus'
    Q_CLASSINFO("D-Bus Interface", "%{NamePrefix}.%{CompanyName}.%{ServiceName}.service")
    @endif
public:
    explicit %{ClassName}(QObject *parent = nullptr);

signals:

public slots:
    void start();
};

#endif // QAUSERSERVICE_H
