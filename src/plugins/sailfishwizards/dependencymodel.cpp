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


#include "dependencymodel.h"

namespace SailfishWizards {
namespace Internal {

ExternalLibrary::ExternalLibrary()
{}

/*!
 * \brief Sets the name for the external library.
 * \param name New name
 */
void ExternalLibrary::setName(const QString &name)
{
    m_name = name;
}

/*!
 * \brief Sets the visible name for the external library.
 * \param layoutName New layout name
 */
void ExternalLibrary::setLayoutName(const QString &layoutName)
{
    m_layoutName = layoutName;
}

/*!
 * \brief Sets the connection list for a .pro-file
 * \param qtList New list.
 */
void ExternalLibrary::setQtList(const QStringList &qtList)
{
    m_qtList = qtList;
}

/*!
 * \brief Sets the list of dependencies.
 * \param dependencyList New dependecy list.
 */
void ExternalLibrary::setDependencyList(const QStringList &dependencyList)
{
    m_dependencyList = dependencyList;
}

/*!
 * \brief Sets the list of basic types.
 * \param types New types.
 */
void ExternalLibrary::setTypesList(const QStringList &types)
{
    m_types = types;
}

/*!
 * \brief Sets heading for yaml dependencies.
 * \param yamlHeader Heading for yaml dependencies.
 */
void ExternalLibrary::setYamlHeader(const QString &yamlHeader)
{
    m_yamlHeader = yamlHeader;
}

/*!
 * \brief Sets heading for spec dependencies.
 * \param specHeader Heading for spec dependencies.
 */
void ExternalLibrary::setSpecHeader(const QString &specHeader)
{
    m_specHeader = specHeader;
}

/*!
 * \brief Returns name.
 */
QString ExternalLibrary::getName() const
{
    return m_name;
}

/*!
 * \brief Returns the name to display.
 */
QString ExternalLibrary::getLayoutName() const
{
    return m_layoutName;
}

/*!
 * \brief Returns a list of dependencies for a .pro-file.
 */
QStringList ExternalLibrary::getQtList() const
{
    return m_qtList;
}

/*!
 * \brief Returns a list of dependencies.
 */
QStringList ExternalLibrary::getDependencyList() const
{
    return m_dependencyList;
}

/*!
 * \brief Returns list Returns a list of basic types.
 */
QStringList ExternalLibrary::getTypesList() const
{
    return m_types;
}

/*!
 * \brief Returns the yaml header.
 */
QString ExternalLibrary::getYamlHeader() const
{
    return m_yamlHeader;
}

/*!
 * \brief Returns the spec header.
 */
QString ExternalLibrary::getSpecHeader() const
{
    return m_specHeader;
}

/*!
 * \brief Removes the dependency list
 * entry for the .pro file.
 * \param index Item number.
 */
void ExternalLibrary::removeElemQtList(int &index)
{
    m_qtList.removeAt(index);
}

/*!
 * \brief Removes the dependency list.
 * \param index Item number
 */
void ExternalLibrary::removeElemDependencyList(int &index)
{
    m_dependencyList.removeAt(index);
}

/*!
 * \brief Constructor, sets the role.
 * \param parent The parent object instance.
 */
DependencyModel::DependencyModel(QObject *parent) :
    QAbstractListModel(parent)
{
}

/*!
 * \brief Returns the field of the external library
 * by the specified parameter.
 * \param index Item index.
 * \param role Role number.
 * \return
 */
QVariant DependencyModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() > m_externalLibrarys.count())
        return QVariant();
    const ExternalLibrary &md = m_externalLibrarys[index.row()];
    if (role == LayoutName)
        return md.getLayoutName();
    else if (role == Name)
        return md.getName();
    else if (role == QtList)
        return md.getQtList();
    else if (role == YamlHeader)
        return md.getYamlHeader();
    else if (role == SpecHeader)
        return md.getSpecHeader();
    else if (role == DependencyList)
        return md.getDependencyList();
    else if (role == Types)
        return md.getTypesList();
    return QVariant();
}

/*!
 * \brief Returns an external library.
 * \param index Item index.
 */
ExternalLibrary DependencyModel::getExternalLibrary(const QModelIndex &index) const
{
    if (index.row() < 0 || index.row() > m_externalLibrarys.count())
        return m_externalLibrarys[0];
    else
        return m_externalLibrarys[index.row()];
}

/*!
 * \brief Returns an external library.
 * \param index Item number.
 */
ExternalLibrary DependencyModel::getExternalLibraryByIndex(int &index) const
{
    if (index < 0 || index > m_externalLibrarys.count())
        return m_externalLibrarys[0];
    else
        return m_externalLibrarys[index];
}

/*!
 * \brief Removes an item from a dependency list
 * for a .pro file for the specified external library.
 * \param indexMd External library number.
 * \param indexEl Dependency number.
 */
void DependencyModel::removeElemQtList(int &indexMd, int &indexEl)
{
    m_externalLibrarys[indexMd].removeElemQtList(indexEl);
}

/*!
 * \brief Removes an item from a dependency list.
 * \param indexMd External library number.
 * \param index Dependency number.
 */
void DependencyModel::removeElemDependencyList(int &indexMd, int &index)
{
    m_externalLibrarys[indexMd].removeElemDependencyList(index);
}

/*!
 * \brief Adds a new external library to the model.
 * \param md New external library
 */
void DependencyModel::addExternalLibrary(const ExternalLibrary &md)
{
    m_externalLibrarys << md;
    emit dataChanged(createIndex(0, 0), createIndex(m_externalLibrarys.size() - 1, 0));
}

/*!
 * \brief Returns the number of model elements.
 */
int DependencyModel::size() const
{
    return m_externalLibrarys.size();
}

/*!
 * \brief Returns the number of rows of the model.
 * \param parent The parent object instance.
 */
int DependencyModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_externalLibrarys.size();
}

/*!
 * \brief Returns a list of dependencies
 * for the .pro file for the selected external library.
 * \param index External library number.
 */
QStringList DependencyModel::getQtList(int &index) const
{
    return m_externalLibrarys[index].getQtList();
}

/*!
 * \brief Returns a list of dependencies.
 * \param index index External library number.
 */
QStringList DependencyModel::getDependencyList(int &index) const
{
    return m_externalLibrarys[index].getDependencyList();
}

/*!
 * \brief Returns a list of basic types.
 * \param index External library number.
 */
QStringList DependencyModel::getTypesList(int &index) const
{
    return m_externalLibrarys[index].getTypesList();
}

/*!
 * \brief Returns the yaml header.
 * \param index External library number.
 */
QString DependencyModel::getYamlHeader(int &index) const
{
    return m_externalLibrarys[index].getYamlHeader();
}

/*!
 * \brief Returns the spec header.
 * \param index External library number.
 */
QString DependencyModel::getSpecHeader(int &index) const
{
    return m_externalLibrarys[index].getSpecHeader();
}

/*!
 * \brief DependencyModel::clear Cleans the model.
 */
void DependencyModel::clear()
{
    m_externalLibrarys.clear();
}

/*!
 * \brief Deletes selected lines.
 * Returns \c true if successful, otherwise \c false.
 * \param pos Start line number.
 * \param count Number of lines.
 * \param parent The parent object instance.
 */
bool DependencyModel::removeRows(int pos, int count, const QModelIndex &parent)
{
    Q_UNUSED(parent);
    beginRemoveRows(QModelIndex(), pos, pos + count - 1);
    for (int row = 0; row < count; row++) {
        m_externalLibrarys.removeAt(pos);
    }
    endRemoveRows();
    return true;
}

} // namespace Internal
} // namespace SailfishWizards
