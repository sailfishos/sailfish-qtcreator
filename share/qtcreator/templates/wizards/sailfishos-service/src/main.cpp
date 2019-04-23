#include <QCoreApplication>
#include <QTimer>

#include "%{HdrFileName}"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    %{ClassName} service;
    QTimer::singleShot(0, &service, &%{ClassName}::start);
    return app.exec();
}
