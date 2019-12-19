/****************************************************************************
**
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

#pragma once

#include <QWizard>

#include <memory>

namespace Sfdk {
class BuildEngine;
}

namespace Mer {
namespace Internal {

class MerTargetManagementPage;
class MerTargetManagementPackagesPage;
class MerTargetManagementProgressPage;
class MerTargetManagementDialog : public QWizard
{
    Q_OBJECT

public:
    enum Action {
        ManagePackages,
        Refresh,
        Synchronize,
    };

public:
    explicit MerTargetManagementDialog(Sfdk::BuildEngine *engine, QWidget *parent = nullptr);
    ~MerTargetManagementDialog() override;

    QString targetName() const;
    void setTargetName(const QString &targetName);

    Action action() const;
    void setAction(Action action);

    QStringList packagesToInstall() const;
    void setPackagesToInstall(const QStringList &packageNames);
    QStringList packagesToRemove() const;
    void setPackagesToRemove(const QStringList &packageNames);

    void reject() override;

private:
    void onCurrentIdChanged(int id);
    void managePackages();
    void refresh();
    void synchronize();

private:
    Sfdk::BuildEngine *m_engine;
    std::unique_ptr<MerTargetManagementPage> m_initialPage;
    std::unique_ptr<MerTargetManagementPackagesPage> m_packagesPage;
    std::unique_ptr<MerTargetManagementProgressPage> m_progressPage;
};

} // namespace Internal
} // namespace Mer
