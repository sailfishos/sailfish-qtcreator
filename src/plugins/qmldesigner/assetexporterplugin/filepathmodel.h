/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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
#include <QAbstractListModel>
#include <QFutureWatcher>

#include "utils/fileutils.h"

#include <memory>
#include <unordered_set>

namespace ProjectExplorer {
class Project;
}

namespace QmlDesigner {
class FilePathModel : public QAbstractListModel
{
    Q_DECLARE_TR_FUNCTIONS(QmlDesigner::FilePathModel)

public:
    FilePathModel(ProjectExplorer::Project *project, QObject *parent = nullptr);
    ~FilePathModel() override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;

    Utils::FilePaths files() const;
private:
    void processProject();

    ProjectExplorer::Project *m_project = nullptr;
    std::unique_ptr<QFutureWatcher<Utils::FilePath>> m_preprocessWatcher;
    std::unordered_set<Utils::FilePath> m_skipped;
    Utils::FilePaths m_files;
};

}
