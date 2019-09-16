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

namespace Mer {
namespace Internal {

class MerQtVersion : public QtSupport::BaseQtVersion
{
public:
    MerQtVersion();
    MerQtVersion(const Utils::FileName &path, bool isAutodetected = false,
                 const QString &autodetectionSource = QString());
    ~MerQtVersion() override;

    void setBuildEngineUri(const QUrl &uri);
    QUrl buildEngineUri() const;
    void setTargetName(const QString &name);
    QString targetName() const;

    MerQtVersion *clone() const override;

    QString type() const override;

    QList<ProjectExplorer::Abi> detectQtAbis() const override;

    QString description() const override;

    QSet<Core::Id> targetDeviceTypes() const;

    QList<ProjectExplorer::Task> validateKit(const ProjectExplorer::Kit *k) override;
    QVariantMap toMap() const override;
    void fromMap(const QVariantMap &data) override;
    void addToEnvironment(const ProjectExplorer::Kit *k, Utils::Environment &env) const override;
    Utils::Environment qmakeRunEnvironment() const override;

protected:
    QList<ProjectExplorer::Task> reportIssuesImpl(const QString &proFile,
                                                  const QString &buildDir) const override;

    QSet<Core::Id> availableFeatures() const override;

private:
    QUrl m_buildEngineUri;
    QString m_targetName;
};

} // Internal
} // Mer

#endif // MERQTVERSION_H
