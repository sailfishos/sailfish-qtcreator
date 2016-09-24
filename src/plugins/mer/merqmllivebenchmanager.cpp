/****************************************************************************
**
** Copyright (C) 2016 Jolla Ltd.
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

#include "merqmllivebenchmanager.h"

#include <QProcess>

#include <projectexplorer/project.h>
#include <projectexplorer/session.h>
#include <utils/qtcassert.h>

#include "mersettings.h"

using namespace ProjectExplorer;

namespace Mer {
namespace Internal {

MerQmlLiveBenchManager *MerQmlLiveBenchManager::m_instance = nullptr;

MerQmlLiveBenchManager::MerQmlLiveBenchManager(QObject *parent)
    : QObject(parent)
    , m_enabled{false}
{
    m_instance = this;

    onBenchLocationChanged();
    connect(MerSettings::instance(), &MerSettings::qmlLiveBenchLocationChanged,
            this, &MerQmlLiveBenchManager::onBenchLocationChanged);
}

MerQmlLiveBenchManager::~MerQmlLiveBenchManager()
{
    m_instance = nullptr;
}

MerQmlLiveBenchManager *MerQmlLiveBenchManager::instance()
{
    QTC_CHECK(m_instance);
    return m_instance;
}

void MerQmlLiveBenchManager::startBench()
{
    if (!instance()->m_enabled)
        return;

    QStringList arguments;
    if (SessionManager::startupProject()) {
        const QString currentProjectDir = SessionManager::startupProject()->projectDirectory().toString();
        arguments.append(currentProjectDir);
    }

    QProcess::startDetached(MerSettings::qmlLiveBenchLocation(), arguments);
}

void MerQmlLiveBenchManager::onBenchLocationChanged()
{
    m_enabled = MerSettings::hasValidQmlLiveBenchLocation();
    if (!m_enabled)
        qWarning() << "QmlLive Bench location invalid or not set. QmlLive Bench management will not work.";
}

} // Internal
} // QmlLiveBench
