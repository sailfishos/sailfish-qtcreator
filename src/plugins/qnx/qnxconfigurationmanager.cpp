/****************************************************************************
**
** Copyright (C) 2016 BlackBerry Limited. All rights reserved.
** Contact: BlackBerry (qt@blackberry.com)
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

#include "qnxconfigurationmanager.h"
#include "qnxconfiguration.h"

#include <coreplugin/icore.h>
#include <utils/persistentsettings.h>

using namespace Utils;

namespace Qnx {
namespace Internal {

const QLatin1String QNXConfigDataKey("QNXConfiguration.");
const QLatin1String QNXConfigCountKey("QNXConfiguration.Count");
const QLatin1String QNXConfigsFileVersionKey("Version");

static FilePath qnxConfigSettingsFileName()
{
    return FilePath::fromString(Core::ICore::userResourcePath() + "/qnx/qnxconfigurations.xml");
}

static QnxConfigurationManager *m_instance = nullptr;

QnxConfigurationManager::QnxConfigurationManager()
{
    m_instance = this;
    m_writer = new PersistentSettingsWriter(qnxConfigSettingsFileName(), "QnxConfigurations");
    restoreConfigurations();
    connect(Core::ICore::instance(), &Core::ICore::saveSettingsRequested,
            this, &QnxConfigurationManager::saveConfigs);
}

QnxConfigurationManager *QnxConfigurationManager::instance()
{
    return m_instance;
}

QnxConfigurationManager::~QnxConfigurationManager()
{
    m_instance = nullptr;
    qDeleteAll(m_configurations);
    delete m_writer;
}

QList<QnxConfiguration *> QnxConfigurationManager::configurations() const
{
    return m_configurations;
}

void QnxConfigurationManager::removeConfiguration(QnxConfiguration *config)
{
    if (m_configurations.removeAll(config)) {
        delete config;
        emit configurationsListUpdated();
    }
}

bool QnxConfigurationManager::addConfiguration(QnxConfiguration *config)
{
    if (!config || !config->isValid())
        return false;

    for (QnxConfiguration *c : qAsConst(m_configurations)) {
        if (c->envFile() == config->envFile())
            return false;
    }

    m_configurations.append(config);
    emit configurationsListUpdated();
    return true;
}

QnxConfiguration *QnxConfigurationManager::configurationFromEnvFile(const FilePath &envFile) const
{
    for (QnxConfiguration *c : m_configurations) {
        if (c->envFile() == envFile)
            return c;
    }

    return nullptr;
}

void QnxConfigurationManager::saveConfigs()
{
    QTC_ASSERT(m_writer, return);
    QVariantMap data;
    data.insert(QLatin1String(QNXConfigsFileVersionKey), 1);
    int count = 0;
    for (QnxConfiguration *config : qAsConst(m_configurations)) {
        QVariantMap tmp = config->toMap();
        if (tmp.isEmpty())
            continue;

        data.insert(QNXConfigDataKey + QString::number(count), tmp);
        ++count;
    }


    data.insert(QLatin1String(QNXConfigCountKey), count);
    m_writer->save(data, Core::ICore::dialogParent());
}

void QnxConfigurationManager::restoreConfigurations()
{
    PersistentSettingsReader reader;
    if (!reader.load(qnxConfigSettingsFileName()))
        return;

    QVariantMap data = reader.restoreValues();
    int count = data.value(QNXConfigCountKey, 0).toInt();
    for (int i = 0; i < count; ++i) {
        const QString key = QNXConfigDataKey + QString::number(i);
        if (!data.contains(key))
            continue;

        const QVariantMap dMap = data.value(key).toMap();
        auto configuration = new QnxConfiguration(dMap);
        addConfiguration(configuration);
    }
}

} // Internal
} // Qnx
