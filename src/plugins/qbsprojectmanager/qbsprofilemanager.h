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

#pragma once

#include "qbsprojectmanager_global.h"

#include <QList>
#include <QVariant>

namespace ProjectExplorer { class Kit; }

namespace QbsProjectManager {
namespace Internal {
class DefaultPropertyProvider;

QString toJSLiteral(const QVariant &val);
QVariant fromJSLiteral(const QString &str);

class QbsProfileManager : public QObject
{
    Q_OBJECT

public:
    QbsProfileManager();
    ~QbsProfileManager() override;

    static QbsProfileManager *instance();

    static QString ensureProfileForKit(const ProjectExplorer::Kit *k);
    static QString profileNameForKit(const ProjectExplorer::Kit *kit);
    static void updateProfileIfNecessary(const ProjectExplorer::Kit *kit);
    enum class QbsConfigOp { Get, Set, Unset }; static QString runQbsConfig(QbsConfigOp op, const QString &key, const QVariant &value = {});

signals:
    void qbsProfilesUpdated();

private:
    void setProfileForKit(const QString &name, const ProjectExplorer::Kit *k);
    void addProfile(const QString &name, const QVariantMap &data);
    void addQtProfileFromKit(const QString &profileName, const ProjectExplorer::Kit *k);
    void addProfileFromKit(const ProjectExplorer::Kit *k);
    void updateAllProfiles();

    void handleKitUpdate(ProjectExplorer::Kit *kit);
    void handleKitRemoval(ProjectExplorer::Kit *kit);

    DefaultPropertyProvider *m_defaultPropertyProvider;
    QList<ProjectExplorer::Kit *> m_kitsToBeSetupForQbs;
};

} // namespace Internal
} // namespace QbsProjectManager
