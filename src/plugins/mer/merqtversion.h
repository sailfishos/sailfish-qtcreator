/****************************************************************************
**
** Copyright (C) 2012-2016 Jolla Ltd.
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

#ifndef MERQTVERSION_H
#define MERQTVERSION_H

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtversionfactory.h>

namespace Mer {
namespace Internal {

class MerQtVersion : public QtSupport::BaseQtVersion
{
    Q_DECLARE_TR_FUNCTIONS(Mer::MerQtVersion)

public:
    MerQtVersion();
    ~MerQtVersion() override;

    void setBuildEngineUri(const QUrl &uri);
    QUrl buildEngineUri() const;
    void setBuildTargetName(const QString &name);
    QString buildTargetName() const;

    QString description() const override;

    QSet<Utils::Id> targetDeviceTypes() const;

    ProjectExplorer::Tasks validateKit(const ProjectExplorer::Kit *k) override;
    QVariantMap toMap() const override;
    void fromMap(const QVariantMap &data) override;
    void addToEnvironment(const ProjectExplorer::Kit *k, Utils::Environment &env) const override;
    Utils::Environment qmakeRunEnvironment() const override;

    bool isValid() const override;
    QString invalidReason() const override;

protected:
    ProjectExplorer::Tasks reportIssuesImpl(const QString &proFile,
                                                  const QString &buildDir) const override;

    QSet<Utils::Id> availableFeatures() const override;

private:
    QUrl m_buildEngineUri;
    QString m_buildTargetName;
};

class MerQtVersionFactory : public QtSupport::QtVersionFactory
{
public:
    MerQtVersionFactory();
    ~MerQtVersionFactory() override;
};

} // Internal
} // Mer

#endif // MERQTVERSION_H
