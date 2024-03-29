/****************************************************************************
**
** Copyright (C) 2014-2016,2018-2019 Jolla Ltd.
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

#include <sfdk/utils.h>
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

    static QString sfdkPath();

    static bool clearBuildEnvironmentByDefault();
    static void setClearBuildEnvironmentByDefault(bool byDefault);

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

    static bool isImportQmakeVariablesEnabled();
    static void setImportQmakeVariablesEnabled(bool enabled);
    static bool isAskImportQmakeVariablesEnabled();
    static void setAskImportQmakeVariablesEnabled(bool enabled);

    static bool signPackagesByDefault();
    static void setSignPackagesByDefault(bool byDefault);
    static Sfdk::GpgKeyInfo signingUser();
    static void setSigningUser(const Sfdk::GpgKeyInfo &signingUser);
    static QString signingPassphraseFile();
    static void setSigningPassphraseFile(const QString &passphraseFile);

signals:
    void clearBuildEnvironmentByDefaultChanged(bool byDefault);
    void rpmValidationByDefaultChanged(bool byDefault);
    void qmlLiveBenchLocationChanged(const QString &location);
    void syncQmlLiveWorkspaceEnabledChanged(bool enabled);
    void askBeforeStartingVmEnabledChanged(bool enabled);
    void askBeforeClosingVmEnabledChanged(bool enabled);
    void importQmakeVariablesEnabledChanged(bool enabled);
    void askImportQmakeVariablesEnabledChanged(bool enabled);
    void signPackagesByDefaultChanged(bool byDefault);
    void defaultSigningUserChanged(const Sfdk::GpgKeyInfo &signingUser);
    void defaultSigningPassphraseFileChanged(const QString &passphraseFile);

private:
    void read();
    void save();

private:
    static MerSettings *s_instance;
    bool m_clearBuildEnvironmentByDefault;
    bool m_rpmValidationByDefault;
    QString m_qmlLiveBenchLocation;
    bool m_syncQmlLiveWorkspaceEnabled;
    bool m_askBeforeStartingVmEnabled;
    bool m_askBeforeClosingVmEnabled;
    bool m_importQmakeVariablesEnabled;
    bool m_askImportQmakeVariablesEnabled;
    bool m_signPackagesByDefault;
    Sfdk::GpgKeyInfo m_defaultSigningUser;
    QString m_defaultSigningPassphraseFile;
};

} // Internal
} // Mer

#endif // MERSETTINGS_H
