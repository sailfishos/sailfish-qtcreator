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
    /*!
     * \brief The constructor of dependency record structure.
     * \param name Name of the dependency.
     * \param requirement Type of version requirement.
     * \param version Version number.
     * \param epoch Epoch number.
     * \param release Release number.
     */
    DependencyRecord(const QString &name = QString(), Requirement requirement = EMPTY, const QString &version = QString(),
                     const QString &epoch = QString(), const QString &release = QString())
        : m_name(name)
        , m_requirement(requirement)
        , m_epoch(epoch)
        , m_version(version)
        , m_release(release)
    {}

    /*!
     * \brief Returns the name of the record.
     */
    QString getName() const
    {
        return m_name;
    }
    /*!
     * \brief Returns the requirement of the record.
     */
    Requirement getRequirement() const
    {
        return m_requirement;
    }
    /*!
     * \brief Returns the epoch of the record.
     */
    QString getEpoch() const
    {
        return m_epoch;
    }
    /*!
     * \brief Returns the version of the record.
     */
    QString getVersion() const
    {
        return m_version;
    }
    /*!
     * \brief Returns the release of the record.
     */
    QString getRelease() const
    {
        return m_release;
    }
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
    /*!
     * \brief This is the default constructor for the model.
     * \param parent Parent object instance.
     */
    DependencyListModel(QObject *parent = nullptr)
        : QAbstractListModel(parent)
    {}
    /*!
     * \brief This is the initializing constructor for the model.
     * It initializes the model with dependency records list.
     * \param dependencies List of dependencies.
     * \param parent Parent object instance.
     */
    DependencyListModel(const QList<DependencyRecord> &dependencies, QObject *parent = nullptr)
        : QAbstractListModel(parent)
        , m_dependencyList(dependencies)
    {}

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role) const;
    bool insertRows(int position, int rows, const QModelIndex &index = QModelIndex());
    bool removeRows(int position, int rows, const QModelIndex &index = QModelIndex());
    void setDependencyList(const QList<DependencyRecord> &dependencies);
    void appendRecord(const DependencyRecord &record);
    void removeRecord(const DependencyRecord &record);
    void removeRecordAt(int position);
    void clearRecords();
    /*!
     * \brief Returns the dependency list that the model contains.
     */
    QList<DependencyRecord> getDependencyList() const
    {
        return m_dependencyList;
    }
    /*!
     * \brief Returns the size of dependency list.
     */
    int size() const
    {
        return m_dependencyList.size();
    }

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
    /*!
     * \brief The constructor of the project component structure
     * \param name Name of the component.
     * \param dependencies List of the models containing different types of dependencies: pkgBr, pkgConfigBr and runtime dependencies.
     */
    ProjectComponent(const QString &name = QString(), QList<DependencyListModel *> dependencies = QList<DependencyListModel *>())
        : m_dependencies(dependencies)
        , m_name(name)
    {}
    /*!
     * \brief Returns the name of the component.
     */
    QString getName() const
    {
        return m_name;
    }
    /*!
     * \brief Returns the lists of dependencies this component contains.
     */
    const QList<DependencyListModel *> getDependencies() const
    {
        return m_dependencies;
    }
    bool operator==(const ProjectComponent &component);

private:
    QList<DependencyListModel *> m_dependencies;
    QString m_name;
};

} // namespace Internal
} // namespace SailfishWizards
