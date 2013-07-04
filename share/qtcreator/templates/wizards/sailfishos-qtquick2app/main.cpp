
#include <QGuiApplication>
#include <QQuickView>

#include "sailfishapplication.h"

Q_DECL_EXPORT int main(int argc, char *argv[])
{
    QScopedPointer<QGuiApplication> app(Sailfish::createApplication(argc, argv));
    QScopedPointer<QQuickView> view(Sailfish::createView("main.qml"));

    Sailfish::showView(view.data());
    
    return app->exec();
}

