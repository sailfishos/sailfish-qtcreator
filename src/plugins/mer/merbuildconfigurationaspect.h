/****************************************************************************
**
** Copyright (C) 2020 Open Mobile Platform LLC.
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

#ifndef MERBUILDCONFIGURATIONASPECTS_H
#define MERBUILDCONFIGURATIONASPECTS_H

#include <projectexplorer/projectconfiguration.h>

#include <QPointer>

namespace ProjectExplorer {
    class BuildConfiguration;
}

namespace Utils {
    class Environment;
}

namespace Mer {
namespace Internal {

class MerBuildConfigurationAspect : public ProjectExplorer::ProjectConfigurationAspect
{
    Q_OBJECT

public:
    explicit MerBuildConfigurationAspect(ProjectExplorer::BuildConfiguration *buildConfiguration);

    ProjectExplorer::BuildConfiguration *buildConfiguration() const;

    QString specFilePath() const { return m_specFilePath; }
    void setSpecFilePath(const QString &specFilePath);

    // While we pretend here and in the UI that these are sfdk options, they are
    // currently passed directly to mb2
    QString sfdkOptionsString() const { return m_sfdkOptionsString; }
    void setSfdkOptionsString(const QString &sfdkOptionsString);

    static QStringList allowedSfdkOptions() { return s_allowedSfdkOptions; }

    void addToEnvironment(Utils::Environment &env) const;

    void fromMap(const QVariantMap &map) override;
    void toMap(QVariantMap &map) const override;

private:
    static const QStringList s_allowedSfdkOptions;
    QPointer<ProjectExplorer::BuildConfiguration> m_buildConfiguration;
    QString m_specFilePath;
    QString m_sfdkOptionsString;
};

} // Internal
} // Mer

#endif // MERBUILDCONFIGURATIONASPECTS_H
