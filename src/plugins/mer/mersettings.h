/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
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

#ifndef MERSETTINGS_H
#define MERSETTINGS_H

#include "meremulatordevice.h"

#include <QtCore/QObject>

namespace Mer {
namespace Internal {

class MerSettings : public QObject
{
    Q_OBJECT
public:
    explicit MerSettings(QObject *parent = 0);
    ~MerSettings() override;

    static MerSettings *instance();

    static QString environmentFilter();
    static void setEnvironmentFilter(const QString &filter);
    static bool isEnvironmentFilterFromEnvironment();

    static bool rpmValidationByDefault();
    static void setRpmValidationByDefault(bool byDefault);

    static QString qmlLiveBenchLocation();
    static void setQmlLiveBenchLocation(const QString &location);
    static bool hasValidQmlLiveBenchLocation();

    static bool isSyncQmlLiveWorkspaceEnabled();
    static void setSyncQmlLiveWorkspaceEnabled(bool enable);

    static bool isAskBeforeStartingVmEnabled();
    static void setAskBeforeStartingVmEnabled(bool enabled);
    static bool isAskBeforeClosingVmEnabled();
    static void setAskBeforeClosingVmEnabled(bool enabled);

    static Utils::FileName globalDeviceModelsFileName();
    static QMap<QString, MerEmulatorDeviceModel> deviceModels();

signals:
    void environmentFilterChanged(const QString &filter);
    void rpmValidationByDefaultChanged(bool byDefault);
    void qmlLiveBenchLocationChanged(const QString &location);
    void syncQmlLiveWorkspaceEnabledChanged(bool enabled);
    void askBeforeStartingVmEnabledChanged(bool enabled);
    void askBeforeClosingVmEnabledChanged(bool enabled);

private:
    void read();
    void save();
    static QMap<QString, MerEmulatorDeviceModel> deviceModelsRead(const Utils::FileName &fileName);

private:
    static MerSettings *s_instance;
    QString m_environmentFilter;
    QString m_environmentFilterFromEnvironment;
    bool m_rpmValidationByDefault;
    QString m_qmlLiveBenchLocation;
    bool m_syncQmlLiveWorkspaceEnabled;
    bool m_askBeforeStartingVmEnabled;
    bool m_askBeforeClosingVmEnabled;
    QMap<QString, MerEmulatorDeviceModel> m_deviceModels;
};

} // Internal
} // Mer

#endif // MERSETTINGS_H
