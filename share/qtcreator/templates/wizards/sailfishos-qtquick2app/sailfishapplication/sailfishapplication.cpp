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

    if(file.contains(":")) {
        view->setSource(QUrl(file));
    } else {
        view->setSource(QUrl::fromLocalFile(deploymentPath() + file));
    }
    return view;
}


QString Sailfish::deploymentRoot()
{
    if(QCoreApplication::applicationFilePath().startsWith("/opt/sdk/")) {
        // Deployed under /opt/sdk/<app-name>/..
        // parse the base path from application binary's path and use it as base
        QString basePath = QCoreApplication::applicationFilePath();
        basePath.chop(basePath.length() -  basePath.indexOf("/", 9)); // first index after /opt/sdk/
        return basePath;
    } else {
        return "";
    }
}

QString Sailfish::deploymentPath()
{
    bool isDesktop = qApp->arguments().contains("-desktop");
    QString path;
    if (isDesktop) {
        path = qApp->applicationDirPath() + QDir::separator();
    } else {
        path = deploymentRoot() + QString(DEPLOYMENT_PATH);
    }

    return path;
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
    if(file.contains(":")) {
        view->setSource(QUrl(file));
    } else {
        view->setSource(QUrl::fromLocalFile(deploymentPath() + file));
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

