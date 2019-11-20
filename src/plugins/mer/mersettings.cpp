/****************************************************************************
**
** Copyright (C) 2014,2016,2018-2019 Jolla Ltd.
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

#include "mersettings.h"

#include <QFileInfo>
#include <QProcessEnvironment>

#include <app/app_version.h>
#include <coreplugin/icore.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/persistentsettings.h>
#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include "merconstants.h"
#include "merlogging.h"

using Core::ICore;
using namespace Utils;
using namespace ExtensionSystem;

namespace Mer {
namespace Internal {

using namespace Constants;

namespace {
const char SETTINGS_CATEGORY[] = "Mer";
const char ENVIRONMENT_FILTER_KEY[] = "EnvironmentFilter";
const char RPM_VALIDATION_BY_DEFAULT_KEY[] = "RpmValidationByDefault";
const char QML_LIVE_BENCH_LOCATION_KEY[] = "QmlLiveBenchLocation";
const char ASK_BEFORE_STARTING_VM[] = "AskBeforeStartingVm";
const char ASK_BEFORE_CLOSING_VM[] = "AskBeforeClosingVm";
const char IMPORT_QMAKE_VARIABLES[] = "ImportQmakeVariables";
const char ASK_IMPORT_QMAKE_VARIABLES[] = "AskImportQmakeVariables";
} // namespace anonymous

MerSettings *MerSettings::s_instance = 0;

MerSettings::MerSettings(QObject *parent)
    : QObject(parent)
    , m_rpmValidationByDefault(true)
    , m_syncQmlLiveWorkspaceEnabled(true)
{
    Q_ASSERT(s_instance == 0);
    s_instance = this;

    read();
}

MerSettings::~MerSettings()
{
#ifdef MER_LIBRARY
    save();
#endif // MER_LIBRARY

    s_instance = 0;
}

MerSettings *MerSettings::instance()
{
    Q_ASSERT(s_instance != 0);

    return s_instance;
}

QString MerSettings::environmentFilter()
{
    if (!s_instance->m_environmentFilterFromEnvironment.isNull())
        return s_instance->m_environmentFilterFromEnvironment;
    else
        return s_instance->m_environmentFilter;
}

void MerSettings::setEnvironmentFilter(const QString &filter)
{
    QTC_CHECK(!isEnvironmentFilterFromEnvironment());

    if (s_instance->m_environmentFilter == filter)
        return;

    s_instance->m_environmentFilter = filter;

    emit s_instance->environmentFilterChanged(s_instance->m_environmentFilter);
}

bool MerSettings::isEnvironmentFilterFromEnvironment()
{
    return !s_instance->m_environmentFilterFromEnvironment.isNull();
}

bool MerSettings::rpmValidationByDefault()
{
    Q_ASSERT(s_instance);

    return s_instance->m_rpmValidationByDefault;
}

void MerSettings::setRpmValidationByDefault(bool byDefault)
{
    Q_ASSERT(s_instance);

    if (s_instance->m_rpmValidationByDefault == byDefault)
        return;

    s_instance->m_rpmValidationByDefault = byDefault;

    emit s_instance->rpmValidationByDefaultChanged(s_instance->m_rpmValidationByDefault);
}

QString MerSettings::qmlLiveBenchLocation()
{
    Q_ASSERT(s_instance);

    return s_instance->m_qmlLiveBenchLocation;
}

void MerSettings::setQmlLiveBenchLocation(const QString &location)
{
    Q_ASSERT(s_instance);

    if (s_instance->m_qmlLiveBenchLocation == location)
        return;

    s_instance->m_qmlLiveBenchLocation = location;

    emit s_instance->qmlLiveBenchLocationChanged(s_instance->m_qmlLiveBenchLocation);
}

bool MerSettings::hasValidQmlLiveBenchLocation()
{
    Q_ASSERT(s_instance);

    return QFileInfo(s_instance->m_qmlLiveBenchLocation).isExecutable();
}

bool MerSettings::isSyncQmlLiveWorkspaceEnabled()
{
    Q_ASSERT(s_instance);

    return s_instance->m_syncQmlLiveWorkspaceEnabled;
}

void MerSettings::setSyncQmlLiveWorkspaceEnabled(bool enable)
{
    Q_ASSERT(s_instance);

    if (s_instance->m_syncQmlLiveWorkspaceEnabled == enable)
        return;

    s_instance->m_syncQmlLiveWorkspaceEnabled = enable;

    emit s_instance->syncQmlLiveWorkspaceEnabledChanged(s_instance->m_syncQmlLiveWorkspaceEnabled);
}

bool MerSettings::isAskBeforeStartingVmEnabled()
{
    Q_ASSERT(s_instance);

    return s_instance->m_askBeforeStartingVmEnabled;
}

void MerSettings::setAskBeforeStartingVmEnabled(bool enabled)
{
    Q_ASSERT(s_instance);

    if (s_instance->m_askBeforeStartingVmEnabled == enabled)
        return;

    s_instance->m_askBeforeStartingVmEnabled = enabled;

    emit s_instance->askBeforeStartingVmEnabledChanged(s_instance->m_askBeforeStartingVmEnabled);
}

bool MerSettings::isAskBeforeClosingVmEnabled()
{
    Q_ASSERT(s_instance);

    return s_instance->m_askBeforeClosingVmEnabled;
}

void MerSettings::setAskBeforeClosingVmEnabled(bool enabled)
{
    Q_ASSERT(s_instance);

    if (s_instance->m_askBeforeClosingVmEnabled == enabled)
        return;

    s_instance->m_askBeforeClosingVmEnabled = enabled;

    emit s_instance->askBeforeClosingVmEnabledChanged(s_instance->m_askBeforeClosingVmEnabled);
}

bool MerSettings::isImportQmakeVariablesEnabled()
{
    Q_ASSERT(s_instance);

    return s_instance->m_importQmakeVariablesEnabled;
}

void MerSettings::setImportQmakeVariablesEnabled(bool enabled)
{
    Q_ASSERT(s_instance);

    if (s_instance->m_importQmakeVariablesEnabled == enabled)
        return;

    s_instance->m_importQmakeVariablesEnabled = enabled;

    emit s_instance->importQmakeVariablesEnabledChanged(s_instance->m_importQmakeVariablesEnabled);
}

bool MerSettings::isAskImportQmakeVariablesEnabled()
{
    Q_ASSERT(s_instance);

    return s_instance->m_askImportQmakeVariablesEnabled;
}

void MerSettings::setAskImportQmakeVariablesEnabled(bool enabled)
{
    Q_ASSERT(s_instance);

    if (s_instance->m_askImportQmakeVariablesEnabled == enabled)
        return;

    s_instance->m_askImportQmakeVariablesEnabled = enabled;

    emit s_instance->askImportQmakeVariablesEnabledChanged(s_instance->m_askImportQmakeVariablesEnabled);
}

void MerSettings::read()
{
#ifdef MER_LIBRARY
    QSettings *settings = ICore::settings();
#else
    auto settings = std::make_unique<QSettings>(QSettings::IniFormat, QSettings::UserScope,
            QLatin1String(Core::Constants::IDE_SETTINGSVARIANT_STR),
            QLatin1String(Core::Constants::IDE_CASED_ID));
#endif // MER_LIBRARY

    settings->beginGroup(QLatin1String(SETTINGS_CATEGORY));

    m_environmentFilter = settings->value(QLatin1String(ENVIRONMENT_FILTER_KEY))
        .toString();
    m_rpmValidationByDefault = settings->value(QLatin1String(RPM_VALIDATION_BY_DEFAULT_KEY),
            true).toBool();
    m_qmlLiveBenchLocation = settings->value(QLatin1String(QML_LIVE_BENCH_LOCATION_KEY)).toString();
    m_askBeforeStartingVmEnabled = settings->value(QLatin1String(ASK_BEFORE_STARTING_VM), true).toBool();
    m_askBeforeClosingVmEnabled = settings->value(QLatin1String(ASK_BEFORE_STARTING_VM), true).toBool();
    m_importQmakeVariablesEnabled = settings->value(QLatin1String(IMPORT_QMAKE_VARIABLES), true).toBool();
    m_askImportQmakeVariablesEnabled = settings->value(QLatin1String(ASK_IMPORT_QMAKE_VARIABLES), true).toBool();

    settings->endGroup();

    if (qEnvironmentVariableIsSet(Constants::SAILFISH_OS_SDK_ENVIRONMENT_FILTER_DEPRECATED)) {
#ifdef MER_LIBRARY // Log::mer would be the missing symbol here
        qCWarning(Log::mer) << "The environment variable"
            << QLatin1String(Constants::SAILFISH_OS_SDK_ENVIRONMENT_FILTER_DEPRECATED)
            << "is deprecated. Use"
            << QLatin1String(Constants::SAILFISH_SDK_ENVIRONMENT_FILTER)
            << "instead";
#endif
        qputenv(Constants::SAILFISH_SDK_ENVIRONMENT_FILTER,
                qgetenv(Constants::SAILFISH_OS_SDK_ENVIRONMENT_FILTER_DEPRECATED));
        qunsetenv(Constants::SAILFISH_OS_SDK_ENVIRONMENT_FILTER_DEPRECATED);
    }

    m_environmentFilterFromEnvironment =
        QProcessEnvironment::systemEnvironment().value(Constants::SAILFISH_SDK_ENVIRONMENT_FILTER);
}

#ifdef MER_LIBRARY
void MerSettings::save()
{
    QSettings *settings = ICore::settings();
    settings->beginGroup(QLatin1String(SETTINGS_CATEGORY));

    settings->setValue(QLatin1String(ENVIRONMENT_FILTER_KEY), m_environmentFilter);
    settings->setValue(QLatin1String(RPM_VALIDATION_BY_DEFAULT_KEY), m_rpmValidationByDefault);
    if (m_qmlLiveBenchLocation.isEmpty())
        settings->remove(QLatin1String(QML_LIVE_BENCH_LOCATION_KEY));
    else
        settings->setValue(QLatin1String(QML_LIVE_BENCH_LOCATION_KEY), m_qmlLiveBenchLocation);
    settings->setValue(QLatin1String(ASK_BEFORE_STARTING_VM), m_askBeforeStartingVmEnabled);
    settings->setValue(QLatin1String(ASK_BEFORE_STARTING_VM), m_askBeforeClosingVmEnabled);
    settings->setValue(QLatin1String(IMPORT_QMAKE_VARIABLES), m_importQmakeVariablesEnabled);
    settings->setValue(QLatin1String(ASK_IMPORT_QMAKE_VARIABLES), m_askImportQmakeVariablesEnabled);

    settings->endGroup();
}
#endif // MER_LIBRARY

} // Internal
} // Mer
