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

#include "qttestsettings.h"

namespace Autotest {
namespace Internal {

static const char metricsKey[]           = "Metrics";
static const char noCrashhandlerKey[]    = "NoCrashhandlerOnDebug";

static MetricsType intToMetrics(int value)
{
    switch (value) {
    case Walltime:
        return Walltime;
    case TickCounter:
        return TickCounter;
    case EventCounter:
        return EventCounter;
    case CallGrind:
        return CallGrind;
    case Perf:
        return Perf;
    default:
        return Walltime;
    }
}

QString QtTestSettings::name() const
{
    return QString("QtTest");
}

void QtTestSettings::fromFrameworkSettings(const QSettings *s)
{
    metrics = intToMetrics(s->value(metricsKey, Walltime).toInt());
    noCrashHandler = s->value(noCrashhandlerKey, true).toBool();
}

void QtTestSettings::toFrameworkSettings(QSettings *s) const
{
    s->setValue(metricsKey, metrics);
    s->setValue(noCrashhandlerKey, noCrashHandler);
}

QString QtTestSettings::metricsTypeToOption(const MetricsType type)
{
    switch (type) {
    case MetricsType::Walltime:
        return QString();
    case MetricsType::TickCounter:
        return QLatin1String("-tickcounter");
    case MetricsType::EventCounter:
        return QLatin1String("-eventcounter");
    case MetricsType::CallGrind:
        return QLatin1String("-callgrind");
    case MetricsType::Perf:
        return QLatin1String("-perf");
    default:
        return QString();
    }
}

} // namespace Internal
} // namespace Autotest
