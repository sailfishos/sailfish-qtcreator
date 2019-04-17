/****************************************************************************
**
** Copyright (C) 2019 Open Mobile Platform LLC
** Contact: https://community.omprussia.ru/open-source
**
** This file is part of Qt Creator.
**
** $QT_BEGIN_LICENSE:LGPL$
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included
** in the packaging of this file. Please review the following information
** to ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: https://www.gnu.org/licenses/lgpl-2.1.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#pragma once

#include <QString>
#include <QStringListModel>

namespace SailfishWizards {
namespace Internal {

/*!
 * \brief This class defines the dependency record structure for the dependency list model.
 * \sa DependencyListModel
 */
class DependencyRecord
{
public:
    enum Requirement {EMPTY, LESS_OR_EQUAL, MORE_OR_EQUAL, EQUAL, LESS, MORE};

    DependencyRecord(const QString &name = QString(), Requirement requirement = EMPTY, const QString &version = QString(),
                     const QString &epoch = QString(), const QString &release = QString());

    QString getName() const;
    Requirement getRequirement() const;
    QString getEpoch() const;
    QString getVersion() const;
    QString getRelease() const;
    QString toString() const;
    DependencyRecord fromString(const QString &dependencyString);
    bool operator==(const DependencyRecord &record);

private:
    QString m_name;
    Requirement m_requirement;
    QString m_epoch;
    QString m_version;
    QString m_release;
};

/*!
 * \brief The DependencyListModel class defines the model of the dependency lists for the spectacle dependencies page.
 * \sa QAbstractTableModel, SpectacleFileDependenciesPage
 */
class DependencyListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    DependencyListModel(QObject *parent = nullptr);
    DependencyListModel(const QList<DependencyRecord> &dependencies, QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool insertRows(int position, int rows, const QModelIndex &index = QModelIndex()) override;
    bool removeRows(int position, int rows, const QModelIndex &index = QModelIndex()) override;

    void setDependencyList(const QList<DependencyRecord> &dependencies);
    void appendRecord(const DependencyRecord &record);
    void removeRecord(const DependencyRecord &record);
    void removeRecordAt(int position);
    void clearRecords();

    QList<DependencyRecord> getDependencyList() const;
    int size() const;

private:
    QList<DependencyRecord> m_dependencyList;
};

/*!
 * \brief This class defines the project component structure for the check component table model.
 * \sa CheckComponentTableModel
 */
class ProjectComponent
{
public:
    ProjectComponent(const QString &name = QString(), QList<DependencyListModel *> dependencies = QList<DependencyListModel *>());
    QString getName() const;
    const QList<DependencyListModel *> getDependencies() const;
    bool operator==(const ProjectComponent &component);

private:
    QList<DependencyListModel *> m_dependencies;
    QString m_name;
};

} // namespace Internal
} // namespace SailfishWizards
