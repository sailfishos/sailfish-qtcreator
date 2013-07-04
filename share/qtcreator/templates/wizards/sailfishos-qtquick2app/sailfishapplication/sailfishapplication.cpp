#include <QGuiApplication>
#include <QDir>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQmlContext>
#include <QQuickView>
#include <QQuickItem>

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
    view = MDeclarativeCache::qQuickView();
#else
    view = new QQuickView;
#endif

    bool isDesktop = qApp->arguments().contains("-desktop");

    QString path;
    if (isDesktop) {
        path = qApp->applicationDirPath() + QDir::separator();
#ifdef DESKTOP
        view->setViewport(new QGLWidget);
#endif
    } else {
        path = QString(DEPLOYMENT_PATH);
    }
    if(file.contains(":")) {
        view->setSource(QUrl(file));
    } else {
        if(QCoreApplication::applicationFilePath().startsWith("/opt/sdk/")) {
            // Quick deployed under /opt/sdk
            // parse the base path from application binary's path and use it as base
            QString basePath = QCoreApplication::applicationFilePath();
            basePath.chop(basePath.length() -  basePath.indexOf("/", 9)); // first index after /opt/sdk/
            view->setSource(QUrl::fromLocalFile(basePath + path + file));
        } else {
            // Otherwise use deployement path as is
            view->setSource(QUrl::fromLocalFile(path + file));
        }
    }
    return view;
}

QQuickView *Sailfish::createView()
{
    QQuickView *view;
#ifdef HAS_BOOSTER
    view = MDeclarativeCache::qQuickView();
#else
    view = new QQuickView;
#endif

    return view;
}

void Sailfish::setView(QQuickView *view, const QString &file)
{
    bool isDesktop = qApp->arguments().contains("-desktop");
    QString path;
    if (isDesktop) {
        path = qApp->applicationDirPath() + QDir::separator();
    } else {
        path = QString(DEPLOYMENT_PATH);
    }
    if(file.contains(":")) {
        view->setSource(QUrl(file));
    } else {
        view->setSource(QUrl::fromLocalFile(path + file));
    }
}

void Sailfish::showView(QQuickView* view) {
    view->setResizeMode(QQuickView::SizeRootObjectToView);

    bool isDesktop = qApp->arguments().contains("-desktop");

    if (isDesktop) {
        view->resize(480, 854);
        view->rootObject()->setProperty("_desktop", true);
        view->show();
    } else {
        view->showFullScreen();
    }
}

