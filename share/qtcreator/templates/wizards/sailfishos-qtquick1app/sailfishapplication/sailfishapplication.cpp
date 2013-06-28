
#include <QGuiApplication>
#include <QDir>

#ifdef DESKTOP
#include <QGLWidget>
#endif

#include <QQmlEngine>
#include <QQuickView>

#ifdef HAS_BOOSTER
#include <MDeclarativeCache>
#endif

#include "sailfishapplication.h"

QGuiApplication *Sailfish::createApplication(int &argc, char **argv)
{
#ifdef HAS_BOOSTER
    return MDeclarativeCache::qApplication(argc, argv);
#else
    return new QGuiApplication(argc, argv);
#endif
}

QQuickView *Sailfish::createView(const QString &file)
{
    QQuickView *view;
#ifdef HAS_BOOSTER
    view = MDeclarativeCache::qDeclarativeView();
#else
    view = new QQuickView;
#endif

    view->setSource(QUrl::fromLocalFile(QString(DEPLOYMENT_PATH) + file));
    
    return view;
}

void Sailfish::showView(QQuickView* view) {

    if (QGuiApplication::platformName() == QLatin1String("qnx") ||
          QGuiApplication::platformName() == QLatin1String("eglfs")) {
        view->setResizeMode(QQuickView::SizeRootObjectToView);
        view->showFullScreen();
    } else {
        view->show();
    }

}
