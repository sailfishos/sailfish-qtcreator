/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include <QDebug>

#include <QApplication>
#include <QStringList>
#include <QFileInfo>

#include <iostream>

#include <stdio.h>
#include <stdlib.h>

#include "iconrenderer/iconrenderer.h"
#include <qt5nodeinstanceclientproxy.h>

#include <QQmlComponent>
#include <QQmlEngine>

#ifdef ENABLE_QT_BREAKPAD
#include <qtsystemexceptionhandler.h>
#endif

#ifdef Q_OS_WIN
#include <Windows.h>
#endif

namespace {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QByteArray localMsg = msg.toLocal8Bit();
    switch (type) {
    case QtDebugMsg:
        fprintf(stderr,
                "Debug: %s (%s:%u, %s)\n",
                localMsg.constData(),
                context.file,
                context.line,
                context.function);
        break;
    case QtInfoMsg:
        fprintf(stderr,
                "Info: %s (%s:%u, %s)\n",
                localMsg.constData(),
                context.file,
                context.line,
                context.function);
        break;
    case QtWarningMsg:
        fprintf(stderr,
                "Warning: %s (%s:%u, %s)\n",
                localMsg.constData(),
                context.file,
                context.line,
                context.function);
        break;
    case QtCriticalMsg:
        fprintf(stderr,
                "Critical: %s (%s:%u, %s)\n",
                localMsg.constData(),
                context.file,
                context.line,
                context.function);
        break;
    case QtFatalMsg:
        fprintf(stderr,
                "Fatal: %s (%s:%u, %s)\n",
                localMsg.constData(),
                context.file,
                context.line,
                context.function);
        abort();
    }
}
#endif

int internalMain(QGuiApplication *application)
{
    QCoreApplication::setOrganizationName("QtProject");
    QCoreApplication::setOrganizationDomain("qt-project.org");
    QCoreApplication::setApplicationName("Qml2Puppet");
    QCoreApplication::setApplicationVersion("1.0.0");

    if (application->arguments().count() < 2
            || (application->arguments().at(1) == "--readcapturedstream" && application->arguments().count() < 3)
            || (application->arguments().at(1) == "--rendericon" && application->arguments().count() < 5)) {
        qDebug() << "Usage:\n";
        qDebug() << "--test";
        qDebug() << "--version";
        qDebug() << "--readcapturedstream <stream file> [control stream file]";
        qDebug() << "--rendericon <icon size> <icon file name> <icon source qml>";

        return -1;
    }

    if (application->arguments().at(1) == "--readcapturedstream" && application->arguments().count() > 2) {
        QFileInfo inputStreamFileInfo(application->arguments().at(2));
        if (!inputStreamFileInfo.exists()) {
            qDebug() << "Input stream does not exist:" << inputStreamFileInfo.absoluteFilePath();

            return -1;
        }

        if (application->arguments().count() > 3) {
            QFileInfo controlStreamFileInfo(application->arguments().at(3));
            if (!controlStreamFileInfo.exists()) {
                qDebug() << "Output stream does not exist:" << controlStreamFileInfo.absoluteFilePath();

                return -1;
            }
        }
    }

    if (application->arguments().count() == 2 && application->arguments().at(1) == "--test") {
        qDebug() << QCoreApplication::applicationVersion();
        QQmlEngine engine;

        QQmlComponent component(&engine);
        component.setData("import QtQuick 2.0\nItem {\n}\n", QUrl::fromLocalFile("test.qml"));

        QObject *object = component.create();

        if (object) {
            qDebug() << "Basic QtQuick 2.0 working...";
        } else {
            qDebug() << "Basic QtQuick 2.0 not working...";
            qDebug() << component.errorString();
        }
        delete object;
        return 0;
    }

    if (application->arguments().count() == 2 && application->arguments().at(1) == "--version") {
        std::cout << 2;
        return 0;
    }

    if (application->arguments().at(1) != "--readcapturedstream" && application->arguments().count() < 4) {
        qDebug() << "Wrong argument count: " << application->arguments().count();
        return -1;
    }

    if (application->arguments().at(1) == "--rendericon") {
        int size = application->arguments().at(2).toInt();
        QString iconFileName = application->arguments().at(3);
        QString iconSource = application->arguments().at(4);

        IconRenderer *iconRenderer = new IconRenderer(size, iconFileName, iconSource);
        iconRenderer->setupRender();

        return application->exec();
    }

#ifdef ENABLE_QT_BREAKPAD
    const QString libexecPath = QCoreApplication::applicationDirPath() + '/' + RELATIVE_LIBEXEC_PATH;
    QtSystemExceptionHandler systemExceptionHandler(libexecPath);
#endif

    new QmlDesigner::Qt5NodeInstanceClientProxy(application);

#if defined(Q_OS_WIN) && defined(QT_NO_DEBUG)
    SetErrorMode(SEM_NOGPFAULTERRORBOX); //We do not want to see any message boxes
#endif

    if (application->arguments().at(1) == "--readcapturedstream")
        return 0;

    return application->exec();
}
} // namespace

int main(int argc, char *argv[])
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    qInstallMessageHandler(myMessageOutput);
#endif
    // Since we always render text into an FBO, we need to globally disable
    // subpixel antialiasing and instead use gray.
    qputenv("QSG_DISTANCEFIELD_ANTIALIASING", "gray");
#ifdef Q_OS_MACOS
    qputenv("QT_MAC_DISABLE_FOREGROUND_APPLICATION_TRANSFORM", "true");
#endif

    //If a style different from Desktop is set we have to use QGuiApplication
    bool useGuiApplication = (!qEnvironmentVariableIsSet("QMLDESIGNER_FORCE_QAPPLICATION")
                              || qgetenv("QMLDESIGNER_FORCE_QAPPLICATION") != "true")
            && qEnvironmentVariableIsSet("QT_QUICK_CONTROLS_STYLE")
            && qgetenv("QT_QUICK_CONTROLS_STYLE") != "Desktop";

    if (useGuiApplication) {
        QGuiApplication application(argc, argv);
        return internalMain(&application);
    } else {
        QApplication application(argc, argv);
        return internalMain(&application);
    }
}
