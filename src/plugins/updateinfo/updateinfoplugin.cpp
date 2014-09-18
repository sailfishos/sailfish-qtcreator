/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "settingspage.h"
#include "updateinfoplugin.h"
#include "updateinfobutton.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/progressmanager/futureprogress.h>
#include <coreplugin/settingsdatabase.h>
#include <utils/qtcassert.h>

#include <QBasicTimer>
#include <QDomDocument>
#include <QFile>
#include <QMenu>

namespace {
    static const quint32 OneMinute = 60000;
}

using namespace Core;

namespace UpdateInfo {
namespace Internal {

class UpdateInfoPluginPrivate
{
public:
    UpdateInfoPluginPrivate()
        : m_settingsPage(0)
    {
    }

    QString updaterProgram;
    QString updaterRunUiArgument;
    QString updaterCheckOnlyArgument;

    QPointer<QProcess> checkUpdatesProcess;
    QPointer<FutureProgress> updateInfoProgress;

    QBasicTimer m_timer;
    QDate m_lastDayChecked;
    QTime m_scheduledUpdateTime;
    SettingsPage *m_settingsPage;
};


UpdateInfoPlugin::UpdateInfoPlugin()
    : d(new UpdateInfoPluginPrivate)
{
}

UpdateInfoPlugin::~UpdateInfoPlugin()
{
    if (d->checkUpdatesProcess) {
        d->checkUpdatesProcess->terminate(); // TODO kill?
        d->checkUpdatesProcess->waitForFinished(-1);
    }

    delete d;
}

bool UpdateInfoPlugin::delayedInitialize()
{
    d->m_timer.start(OneMinute, this);
    return true;
}

void UpdateInfoPlugin::extensionsInitialized()
{
}

bool UpdateInfoPlugin::initialize(const QStringList & /* arguments */, QString *errorMessage)
{
    loadSettings();
    if (d->updaterProgram.isEmpty()) {
        *errorMessage = tr("Could not determine location of maintenance tool. Please check "
            "your installation if you did not enable this plugin manually.");
        return false;
    }

    if (!QFile::exists(d->updaterProgram)) {
        *errorMessage = tr("Could not find maintenance tool at '%1'. Check your installation.")
            .arg(d->updaterProgram);
        return false;
    }

    d->m_settingsPage = new SettingsPage(this);
    addAutoReleasedObject(d->m_settingsPage);

    ActionContainer *const container = ActionManager::actionContainer(Core::Constants::M_HELP);
    container->menu()->addAction(tr("Start Updater"), this, SLOT(startUpdaterUiApplication()));

    return true;
}

void UpdateInfoPlugin::loadSettings()
{
    QSettings *qs = ICore::settings();
    if (qs->contains(QLatin1String("Updater/Application"))) {
        settingsHelper(qs);
        qs->remove(QLatin1String("Updater"));
        saveSettings(); // update to the new settings location
    } else {
        settingsHelper(ICore::settingsDatabase());
    }
}

void UpdateInfoPlugin::saveSettings()
{
    SettingsDatabase *settings = ICore::settingsDatabase();
    if (settings) {
        settings->beginTransaction();
        settings->beginGroup(QLatin1String("Updater"));
        settings->setValue(QLatin1String("Application"), d->updaterProgram);
        settings->setValue(QLatin1String("LastDayChecked"), d->m_lastDayChecked);
        settings->setValue(QLatin1String("RunUiArgument"), d->updaterRunUiArgument);
        settings->setValue(QLatin1String("CheckOnlyArgument"), d->updaterCheckOnlyArgument);
        settings->setValue(QLatin1String("ScheduledUpdateTime"), d->m_scheduledUpdateTime);
        settings->endGroup();
        settings->endTransaction();
    }
}

QTime UpdateInfoPlugin::scheduledUpdateTime() const
{
    return d->m_scheduledUpdateTime;
}

void UpdateInfoPlugin::setScheduledUpdateTime(const QTime &time)
{
    d->m_scheduledUpdateTime = time;
}

// -- protected

void UpdateInfoPlugin::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == d->m_timer.timerId()) {
        const QDate today = QDate::currentDate();
        if ((d->m_lastDayChecked == today) || d->checkUpdatesProcess)
            return; // we checked already or the update task is still running

        bool check = false;
        if (d->m_lastDayChecked <= today.addDays(-2))
            check = true;   // we haven't checked since some days, force check

        if (QTime::currentTime() > d->m_scheduledUpdateTime)
            check = true; // we are behind schedule, force check

        if (check) {
            startUpdaterCheckOnly();
        }
    } else {
        // not triggered from our timer
        ExtensionSystem::IPlugin::timerEvent(event);
    }
}

// -- private slots

void UpdateInfoPlugin::onCheckUpdatesError(QProcess::ProcessError error)
{
    d->m_lastDayChecked = QDate::currentDate();
    saveSettings();

    qWarning() << "Could not execute updater application. Error:" << error;
    delete d->checkUpdatesProcess;
}

void UpdateInfoPlugin::onCheckUpdatesFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    d->m_lastDayChecked = QDate::currentDate();
    saveSettings();

    if (exitStatus != QProcess::NormalExit) {
        qWarning() << "Updater application crashed";
        delete d->checkUpdatesProcess;
        return;
    } else if (exitCode != 0) {
        delete d->checkUpdatesProcess;
        return;
    }

    QByteArray output = d->checkUpdatesProcess->readAllStandardOutput();
    delete d->checkUpdatesProcess;

    QDomDocument updatesDomDocument;
    QString error;
    if (!updatesDomDocument.setContent(output, &error)) {
        qWarning() << "Error parsing updater application output:" << error;
        return;
    }

    if (updatesDomDocument.isNull() || !updatesDomDocument.firstChildElement().hasChildNodes())
        return;

    // add a task to the progress manager
    QFutureInterface<void> fakeTask;
    fakeTask.reportFinished();
    d->updateInfoProgress = ProgressManager::addTask(fakeTask.future(), tr("Updates available"),
            "Update.GetInfo", ProgressManager::KeepOnFinish);
    d->updateInfoProgress->setKeepOnFinish(FutureProgress::KeepOnFinish);

    UpdateInfoButton *button = new UpdateInfoButton();
    d->updateInfoProgress->setWidget(button);
    connect(button, SIGNAL(released()), this, SLOT(startUpdaterUiApplication()));
}

void UpdateInfoPlugin::startUpdaterUiApplication()
{
    QProcess::startDetached(d->updaterProgram, QStringList() << d->updaterRunUiArgument);
    if (!d->updateInfoProgress.isNull())  //this is fading out the last update info
        d->updateInfoProgress->setKeepOnFinish(FutureProgress::HideOnFinish);
}

// -- private

void UpdateInfoPlugin::startUpdaterCheckOnly()
{
    QTC_ASSERT(!d->checkUpdatesProcess, return);

    d->checkUpdatesProcess = new QProcess(this);
    connect(d->checkUpdatesProcess, SIGNAL(error(QProcess::ProcessError)),
            this, SLOT(onCheckUpdatesError(QProcess::ProcessError)));
    connect(d->checkUpdatesProcess, SIGNAL(finished(int,QProcess::ExitStatus)),
            this, SLOT(onCheckUpdatesFinished(int,QProcess::ExitStatus)));
    d->checkUpdatesProcess->start(d->updaterProgram, QStringList() << d->updaterCheckOnlyArgument);
}

template <typename T>
void UpdateInfoPlugin::settingsHelper(T *settings)
{
    settings->beginGroup(QLatin1String("Updater"));

    d->updaterProgram = settings->value(QLatin1String("Application")).toString();
    d->m_lastDayChecked = settings->value(QLatin1String("LastDayChecked"), QDate()).toDate();
    d->updaterRunUiArgument = settings->value(QLatin1String("RunUiArgument"),
        QLatin1String("--updater")).toString();
    d->updaterCheckOnlyArgument = settings->value(QLatin1String("CheckOnlyArgument"),
        QLatin1String("--checkupdates")).toString();
    d->m_scheduledUpdateTime = settings->value(QLatin1String("ScheduledUpdateTime"), QTime(12, 0))
        .toTime();

    settings->endGroup();
}

} //namespace Internal
} //namespace UpdateInfo

Q_EXPORT_PLUGIN(UpdateInfo::Internal::UpdateInfoPlugin)
