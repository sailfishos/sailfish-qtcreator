/****************************************************************************
**
** Copyright (C) 2019 Jolla Ltd.
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

#pragma once

#include <qmakeprojectmanager/qmakebuildconfiguration.h>

#include <QBasicTimer>
#include <QMessageBox>
#include <QPointer>

namespace Mer {
namespace Internal {

class MerQmakeBuildConfiguration : public QmakeProjectManager::QmakeBuildConfiguration
{
    Q_OBJECT

public:
    MerQmakeBuildConfiguration(ProjectExplorer::Target *target, Utils::Id id);
    bool fromMap(const QVariantMap &map) override;

    QList<ProjectExplorer::NamedWidget *> createSubConfigWidgets() override;
    void addToEnvironment(Utils::Environment &env) const override;

    void doInitialize(const ProjectExplorer::BuildInfo &info) override;

protected:
    void timerEvent(QTimerEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    bool isReallyActive() const;
    void maybeSetupExtraParserArguments();
    void setupExtraParserArguments();
    void maybeUpdateExtraParserArguments(bool now = false);
    void updateExtraParserArguments();

private:
    QBasicTimer m_maybeUpdateExtraParserArgumentsTimer;
    QPointer<QMessageBox> m_qmakeQuestion;
};

class MerQmakeBuildConfigurationFactory : public QmakeProjectManager::QmakeBuildConfigurationFactory
{

public:
    MerQmakeBuildConfigurationFactory();
};

} // namespace Internal
} // namespace Mer
