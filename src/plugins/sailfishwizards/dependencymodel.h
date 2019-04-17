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

#include <QList>
#include <QVariant>
#include <QModelIndex>
namespace SailfishWizards {
namespace Internal {
/*!
 * \brief Class for external library.
 */
class ExternalLibrary
{
public:
    ExternalLibrary() {}
    void setName(const QString &name);
    void setLayoutName (const QString &layoutName);
    void setQtList(const QStringList &qtList);
    void setDependencyList (const QStringList &dependencyList);
    void setTypesList(const QStringList &types);
    void setYamlHeader(const QString &yamlHeader);
    void setSpecHeader(const QString &specHeader);
    QString getName() const;
    QString getLayoutName() const;
    QStringList getQtList() const;
    QStringList getDependencyList() const;
    QStringList getTypesList() const;
    QString getYamlHeader() const;
    QString getSpecHeader() const;
    void removeElemQtList(int &index);
    void removeElemDependencyList(int &index);
private:
    QString m_name;
    QString m_layoutName;
    QStringList m_qtList;
    QString m_yamlHeader;
    QString m_specHeader;
    QStringList m_dependencyList;
    QStringList m_types;
};

/*!
 * \brief Class for the list of external libraries
 *
 * \sa QAbstractListModel
 */
class DependencyModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit DependencyModel(QObject *parent = nullptr);
    enum ExternalLibraryRolse {
        LayoutName = Qt::DisplayRole,
        Name = Qt::UserRole + 1,
        QtList = Qt::UserRole + 2,
        YamlHeader = Qt::UserRole + 3,
        SpecHeader = Qt::UserRole + 4,
        DependencyList = Qt::UserRole + 5,
        Types = Qt::UserRole + 6
    };
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    int rowCount(const QModelIndex &parent) const;
    ExternalLibrary getExternalLibrary(const QModelIndex &index) const;
    void addExternalLibrary(const ExternalLibrary &md);
    void removeElemQtList(int &indexMd, int &indexEl);
    int size();
    QStringList getQtList(int &index);
    QStringList getDependencyList(int &index);
    QStringList getTypesList(int &index);
    QString getYamlHeader(int &index);
    QString getSpecHeader(int &index);
    void removeElemDependencyList(int &indexMd, int &index);
    void clear();
    ExternalLibrary getExternalLibraryByIndex(int &index);
    bool removeRows(int pos, int count, const QModelIndex &parent = QModelIndex()) override;
private:
    QList<ExternalLibrary> m_externalLibrarys;
};

} // namespace Internal
} // namespace SailfishWizards
