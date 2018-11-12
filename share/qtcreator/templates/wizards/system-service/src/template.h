#ifndef QAUSERSERVICE_H
#define QAUSERSERVICE_H

#include <QObject>

class %{ProjectName} : public QObject
{
    Q_OBJECT
    @if '%{ServiceType}' === 'dbus'
    Q_CLASSINFO("D-Bus Interface", "%{CompanyName}.%{ProjectName}.service")
    @endif
public:
    explicit %{ProjectName}(QObject *parent = nullptr);

signals:

public slots:
    void start();

    Q_SCRIPTABLE void launchApp(const QString &appName, const QStringList &arguments);
};

#endif // QAUSERSERVICE_H
