/****************************************************************************
**
** Copyright (C) 2019 Jolla Ltd.
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

#pragma once

#include <qmakeprojectmanager/qmakebuildconfiguration.h>

#include <QMessageBox>
#include <QPointer>

namespace Mer {
namespace Internal {

class MerBuildConfiguration : public QmakeProjectManager::QmakeBuildConfiguration
{
    Q_OBJECT

public:
    MerBuildConfiguration(ProjectExplorer::Target *target, Core::Id id);
    void initialize(const ProjectExplorer::BuildInfo &info) override;
    bool fromMap(const QVariantMap &map) override;

private:
    bool isReallyActive() const;
    void setupExtraParserArguments();
    void maybeUpdateExtraParserArguments();
    void updateExtraParserArguments();

private:
    QPointer<QMessageBox> m_qmakeQuestion;
};

class MerBuildConfigurationFactory : public QmakeProjectManager::QmakeBuildConfigurationFactory
{
public:
    MerBuildConfigurationFactory();
};

} // namespace Internal
} // namespace Mer
