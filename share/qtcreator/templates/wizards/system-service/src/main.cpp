#include <QCoreApplication>
#include <QTimer>

#include "%{ProjectName}.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    %{ProjectName} service;
    QTimer::singleShot(0, &service, &%{ProjectName}::start);
    return app.exec();
}
