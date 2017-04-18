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

#include "gettingstartedwelcomepage.h"

#include "exampleslistmodel.h"
#include "screenshotcropper.h"
#include "copytolocationdialog.h"

#include <utils/pathchooser.h>
#include <utils/winutils.h>

#include <coreplugin/coreconstants.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/helpmanager.h>
#include <coreplugin/modemanager.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/project.h>

#include <QMutex>
#include <QThread>
#include <QMutexLocker>
#include <QPointer>
#include <QWaitCondition>
#include <QDir>
#include <QBuffer>
#include <QImage>
#include <QImageReader>
#include <QGridLayout>
#include <QLabel>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QMessageBox>
#include <QApplication>
#include <QQuickImageProvider>
#include <QQmlEngine>
#include <QQmlContext>
#include <QDesktopServices>

using namespace Utils;

namespace QtSupport {
namespace Internal {

const char C_FALLBACK_ROOT[] = "ProjectsFallbackRoot";

QPointer<ExamplesListModel> &examplesModelStatic()
{
    static QPointer<ExamplesListModel> s_examplesModel;
    return s_examplesModel;
}

class Fetcher : public QObject
{
    Q_OBJECT

public:
    Fetcher() : QObject(),  m_shutdown(false)
    {
        connect(Core::ICore::instance(), &Core::ICore::coreAboutToClose, this, &Fetcher::shutdown);
    }

    void wait()
    {
        if (QThread::currentThread() == QApplication::instance()->thread())
            return;
        if (m_shutdown)
            return;

        m_waitcondition.wait(&m_mutex, 4000);
    }

    QByteArray data()
    {
        QMutexLocker lock(&m_dataMutex);
        return m_fetchedData;
    }

    void clearData()
    {
        QMutexLocker lock(&m_dataMutex);
        m_fetchedData.clear();
    }

    bool asynchronousFetchData(const QUrl &url)
    {
        QMutexLocker lock(&m_mutex);

        if (!QMetaObject::invokeMethod(this,
                                       "fetchData",
                                       Qt::AutoConnection,
                                       Q_ARG(QUrl, url))) {
            return false;
        }

        wait();
        return true;
    }


public slots:
    void fetchData(const QUrl &url)
    {
        if (m_shutdown)
            return;

        QMutexLocker lock(&m_mutex);

        if (Core::HelpManager::instance()) {
            QMutexLocker dataLock(&m_dataMutex);
            m_fetchedData = Core::HelpManager::fileData(url);
        }
        m_waitcondition.wakeAll();
    }

private:
    void shutdown()
    {
        m_shutdown = true;
    }

public:
    QByteArray m_fetchedData;
    QWaitCondition m_waitcondition;
    QMutex m_mutex;     //This mutex synchronises the wait() and wakeAll() on the wait condition.
                        //We have to ensure that wakeAll() is called always after wait().

    QMutex m_dataMutex; //This mutex synchronises the access of m_fectedData.
                        //If the wait condition timeouts we otherwise get a race condition.
    bool m_shutdown;
};

class HelpImageProvider : public QQuickImageProvider
{
public:
    HelpImageProvider()
        : QQuickImageProvider(QQuickImageProvider::Image)
    {
    }

    // gets called by declarative in separate thread
    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize)
    {
        QMutexLocker lock(&m_mutex);

        QUrl url = QUrl::fromEncoded(id.toLatin1());


        if (!m_fetcher.asynchronousFetchData(url) || m_fetcher.data().isEmpty()) {
            if (size) {
                size->setWidth(0);
                size->setHeight(0);
            }
            return QImage();
        }

        QByteArray data = m_fetcher.data();
        QBuffer imgBuffer(&data);
        imgBuffer.open(QIODevice::ReadOnly);
        QImageReader reader(&imgBuffer);
        QImage img = reader.read();

        m_fetcher.clearData();
        img = ScreenshotCropper::croppedImage(img, id, requestedSize);
        if (size)
            *size = img.size();
        return img;

    }
private:
    Fetcher m_fetcher;
    QMutex m_mutex;
};

ExamplesWelcomePage::ExamplesWelcomePage()
    : m_engine(0),  m_showExamples(false)
{
}

void ExamplesWelcomePage::setShowExamples(bool showExamples)
{
    m_showExamples = showExamples;
}

QString ExamplesWelcomePage::title() const
{
    if (m_showExamples)
        return tr("Examples");
    else
        return tr("Tutorials");
}

 int ExamplesWelcomePage::priority() const
 {
     if (m_showExamples)
         return 30;
     else
         return 40;
 }

 bool ExamplesWelcomePage::hasSearchBar() const
 {
     if (m_showExamples)
         return true;
     else
         return false;
 }

QUrl ExamplesWelcomePage::pageLocation() const
{
    // normalize paths so QML doesn't freak out if it's wrongly capitalized on Windows
    const QString resourcePath = Utils::FileUtils::normalizePathName(Core::ICore::resourcePath());
    if (m_showExamples)
        return QUrl::fromLocalFile(resourcePath + QLatin1String("/welcomescreen/examples.qml"));
    else
        return QUrl::fromLocalFile(resourcePath + QLatin1String("/welcomescreen/tutorials.qml"));
}

void ExamplesWelcomePage::facilitateQml(QQmlEngine *engine)
{
    m_engine = engine;
    m_engine->addImageProvider(QLatin1String("helpimage"), new HelpImageProvider);
    ExamplesListModelFilter *proxy = new ExamplesListModelFilter(examplesModel(), this);

    proxy->setDynamicSortFilter(true);
    proxy->sort(0);
    proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);

    QQmlContext *rootContenxt = m_engine->rootContext();
    if (m_showExamples) {
        proxy->setShowTutorialsOnly(false);
        rootContenxt->setContextProperty(QLatin1String("examplesModel"), proxy);
        rootContenxt->setContextProperty(QLatin1String("exampleSetModel"), proxy->exampleSetModel());
    } else {
        rootContenxt->setContextProperty(QLatin1String("tutorialsModel"), proxy);
    }
    rootContenxt->setContextProperty(QLatin1String("gettingStarted"), this);
}

Core::Id ExamplesWelcomePage::id() const
{
    return m_showExamples ? "Examples" : "Tutorials";
}

void ExamplesWelcomePage::openHelpInExtraWindow(const QUrl &help)
{
    Core::HelpManager::handleHelpRequest(help, Core::HelpManager::ExternalHelpAlways);
}

void ExamplesWelcomePage::openHelp(const QUrl &help)
{
    Core::HelpManager::handleHelpRequest(help, Core::HelpManager::HelpModeAlways);
}

void ExamplesWelcomePage::openUrl(const QUrl &url)
{
    QDesktopServices::openUrl(url);
}

QString ExamplesWelcomePage::copyToAlternativeLocation(const QFileInfo& proFileInfo, QStringList &filesToOpen, const QStringList& dependencies)
{
    const QString projectDir = proFileInfo.canonicalPath();
    QSettings *settings = Core::ICore::settings();
    CopyToLocationDialog d(Core::ICore::mainWindow());
    d.setSourcePath(projectDir);
    d.setDestinationPath(settings->value(QString::fromLatin1(C_FALLBACK_ROOT),
                                         Core::DocumentManager::projectsDirectory()).toString());

    while (QDialog::Accepted == d.exec()) {
        QString exampleDirName = proFileInfo.dir().dirName();
        QString destBaseDir = d.destinationPath();
        settings->setValue(QString::fromLatin1(C_FALLBACK_ROOT), destBaseDir);
        QDir toDirWithExamplesDir(destBaseDir);
        if (toDirWithExamplesDir.cd(exampleDirName)) {
            toDirWithExamplesDir.cdUp(); // step out, just to not be in the way
            QMessageBox::warning(Core::ICore::mainWindow(), tr("Cannot Use Location"),
                                 tr("The specified location already contains \"%1\" directory. "
                                    "Please specify a valid location.").arg(exampleDirName),
                                 QMessageBox::Ok, QMessageBox::NoButton);
            continue;
        } else {
            QString error;
            QString targetDir = destBaseDir + QLatin1Char('/') + exampleDirName;
            if (FileUtils::copyRecursively(FileName::fromString(projectDir),
                    FileName::fromString(targetDir), &error)) {
                // set vars to new location
                const QStringList::Iterator end = filesToOpen.end();
                for (QStringList::Iterator it = filesToOpen.begin(); it != end; ++it)
                    it->replace(projectDir, targetDir);

                foreach (const QString &dependency, dependencies) {
                    FileName targetFile = FileName::fromString(targetDir);
                    targetFile.appendPath(QDir(dependency).dirName());
                    if (!FileUtils::copyRecursively(FileName::fromString(dependency), targetFile,
                            &error)) {
                        QMessageBox::warning(Core::ICore::mainWindow(), tr("Cannot Copy Project"), error);
                        continue;
                    }
                }


                return targetDir + QLatin1Char('/') + proFileInfo.fileName();
            } else {
                QMessageBox::warning(Core::ICore::mainWindow(), tr("Cannot Copy Project"), error);
                continue;
            }

        }
    }
    return QString();

}

void ExamplesWelcomePage::openProject(const QString &projectFile,
                                      const QStringList &additionalFilesToOpen,
                                      const QString &mainFile,
                                      const QUrl &help,
                                      const QStringList &dependencies,
                                      const QStringList &platforms,
                                      const QStringList &preferredFeatures)
{
    QString proFile = projectFile;
    if (proFile.isEmpty())
        return;

    QStringList filesToOpen = additionalFilesToOpen;
    if (!mainFile.isEmpty()) {
        // ensure that the main file is opened on top (i.e. opened last)
        filesToOpen.removeAll(mainFile);
        filesToOpen.append(mainFile);
    }

    QFileInfo proFileInfo(proFile);
    if (!proFileInfo.exists())
        return;

    QFileInfo pathInfo(proFileInfo.path());
    // If the Qt is a distro Qt on Linux, it will not be writable, hence compilation will fail
    if (!proFileInfo.isWritable()
            || !pathInfo.isWritable() /* path of .pro file */
            || !QFileInfo(pathInfo.path()).isWritable() /* shadow build directory */) {
        proFile = copyToAlternativeLocation(proFileInfo, filesToOpen, dependencies);
    }

    // don't try to load help and files if loading the help request is being cancelled
    if (proFile.isEmpty())
        return;
    ProjectExplorer::ProjectExplorerPlugin::OpenProjectResult result =
            ProjectExplorer::ProjectExplorerPlugin::instance()->openProject(proFile);
    if (result) {
        Core::ICore::openFiles(filesToOpen);
        if (result.project()->needsConfiguration() && (!platforms.isEmpty() || !preferredFeatures.isEmpty())) {
            result.project()->configureAsExampleProject(Core::Id::fromStringList(platforms),
                                                        Core::Id::fromStringList(preferredFeatures));
        }
        Core::ModeManager::activateMode(Core::Constants::MODE_EDIT);
        if (help.isValid())
            openHelpInExtraWindow(help.toString());
        Core::ModeManager::activateMode(ProjectExplorer::Constants::MODE_SESSION);
    } else {
        ProjectExplorer::ProjectExplorerPlugin::showOpenProjectError(result);
    }
}

ExamplesListModel *ExamplesWelcomePage::examplesModel() const
{
    if (examplesModelStatic())
        return examplesModelStatic().data();

    examplesModelStatic() = new ExamplesListModel(const_cast<ExamplesWelcomePage*>(this));
    return examplesModelStatic().data();
}

} // namespace Internal
} // namespace QtSupport

#include "gettingstartedwelcomepage.moc"
