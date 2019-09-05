/****************************************************************************
**
** Copyright (C) 2019 Open Mobile Platform LLC.
** Contact: http://jolla.com/
**
** This file is part of Qt Creator.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Digia.
**
****************************************************************************/

#include "sdk_p.h"

#include "sfdkconstants.h"
#include "virtualboxmanager_p.h"

#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QCoreApplication>
#include <QFileInfo>
#include <QSettings>

using namespace Utils;

namespace Sfdk {

/*!
 * \class Sdk
 */

Sdk *Sdk::s_instance = nullptr;

Sdk::Sdk(Options options)
    : d_ptr(std::make_unique<SdkPrivate>(this))
{
    Q_ASSERT(!s_instance);
    s_instance = this;

    Q_D(Sdk);

    qCDebug(lib) << "Initializing SDK. Options:" << options;

    d->options_ = options;
    d->virtualBoxManager = std::make_unique<VirtualBoxManager>(this);
}

Sdk::~Sdk()
{
    s_instance = nullptr;
}

void Sdk::enableUpdates()
{
    QTC_ASSERT(SdkPrivate::isVersionedSettingsEnabled(), return);
    qCDebug(lib) << "Begin enable udates";
    s_instance->d_func()->updatesEnabled = true;
    emit s_instance->d_func()->enableUpdatesRequested();
    qCDebug(lib) << "End enable updates";
}

bool Sdk::saveSettings(QStringList *errorStrings)
{
    QTC_ASSERT(errorStrings, return false);
    qCDebug(lib) << "Begin save settings";
    emit s_instance->d_func()->saveSettingsRequested(errorStrings);
    qCDebug(lib) << "End save settings. Success:" << errorStrings->isEmpty();
    return errorStrings->isEmpty();
}

/*!
 * \class SdkPrivate
 */

SdkPrivate::SdkPrivate(Sdk *q)
    : QObject(q)
{
}

SdkPrivate::~SdkPrivate() = default;

Utils::FileName SdkPrivate::settingsFile(SettingsScope scope, const QString &basename)
{
    const QString prefix = scope == SessionScope
        ? QString::fromLatin1(Constants::LIB_ID) + '-'
        : QString();
    return settingsLocation(scope).appendPath(prefix + basename);
}

Utils::FileName SdkPrivate::settingsLocation(SettingsScope scope)
{
    static FileName systemLocation;
    static FileName userLocation;
    static FileName sessionLocation;

    FileName *location = nullptr;
    switch (scope) {
    case SystemScope:
        location = &systemLocation;
        break;
    case UserScope:
        location = &userLocation;
        break;
    case SessionScope:
        location = &sessionLocation;
        break;
    }
    Q_ASSERT(location);

    if (!location->isNull())
        return *location;

    QTC_CHECK(!QCoreApplication::organizationName().isEmpty());
    QTC_CHECK(!QCoreApplication::applicationName().isEmpty());

    const QSettings::Scope qscope = scope == SystemScope
        ? QSettings::SystemScope
        : QSettings::UserScope;
    const QString applicationName = scope == SessionScope
        ? QCoreApplication::applicationName()
        : QString::fromLatin1(Constants::LIB_ID);

    QSettings settings(QSettings::IniFormat, qscope, QCoreApplication::organizationName(),
            applicationName);

    // See ICore::userResourcePath()
    QTC_CHECK(settings.fileName().endsWith(".ini"));
    const auto iniInfo = QFileInfo(settings.fileName());
    const QString resourceDir = iniInfo.completeBaseName().toLower();
    *location = FileName::fromString(iniInfo.path() + '/' + resourceDir);

    qCDebug(lib) << "Settings location" << scope << *location;

    return *location;
}

} // namespace Sfdk
