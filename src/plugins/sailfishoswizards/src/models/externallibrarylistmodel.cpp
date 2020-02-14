/*
 * Qt Creator plugin with wizards of the Sailfish OS projects.
 * Copyright © 2020 FRUCT LLC.
 */

#include "models/externallibrarylistmodel.h"

namespace SailfishOSWizards {
namespace Internal {

/*!
 * \brief Constructor initializes a list of model roles.
 * \param parent Parent object instance.
 */
ExternalLibraryListModel::ExternalLibraryListModel(QObject *parent) : QAbstractListModel(parent),
    m_roles{{LayoutName, "layoutName"}, {Name, "name"}, {QtList, "qt"}, {YamlHeader, "yamlHeader"},
    {SpecHeader, "specHeader"}, {DependencyList, "dependencies"}, {Types, "types"}}
{
}

/*!
 * \brief Returns a field of the external library by the specified role.
 * \param index Index to retrieve the external library data.
 * \param role Role number.
 * \return Exteranl library data.
 */
QVariant ExternalLibraryListModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() > m_externalLibraries.count())
        return QVariant();
    ExternalLibrary externalLibrary = m_externalLibraries[index.row()];
    if (role == LayoutName) {
        return externalLibrary.getLayoutName();
    } else if (role == Name) {
        return externalLibrary.getName();
    } else if (role == QtList) {
        return externalLibrary.getQtList();
    } else if (role == YamlHeader) {
        return externalLibrary.getYamlHeader();
    } else if (role == SpecHeader) {
        return externalLibrary.getSpecHeader();
    } else if (role == DependencyList) {
        return externalLibrary.getDependencyList();
    } else if (role == Types) {
        return externalLibrary.getTypesList();
    }
    return QVariant();
}

/*!
 * \brief Returns the number of rows of the model.
 * \param parent The parent object instance.
 */
int ExternalLibraryListModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_externalLibraries.size();
}

/*!
 * \brief Deletes selected rows from the list model.
 * \param pos Start row number to remove.
 * \param count Count of rows to remove.
 * \param parent Parent object instance.
 * \return True if operation is done successfully, false — if not.
 */
bool ExternalLibraryListModel::removeRows(int pos, int count, const QModelIndex &parent)
{
    Q_UNUSED(parent)
    beginRemoveRows(QModelIndex(), pos, pos + count - 1);
    for (int row = 0; row < count; row++) {
        m_externalLibraries.removeAt(pos);
    }
    endRemoveRows();
    return true;
}

/*!
 * \return Number of model elements.
 */
int ExternalLibraryListModel::size() const
{
    return m_externalLibraries.size();
}

/*!
 * \brief Clears the model.
 */
void ExternalLibraryListModel::clear()
{
    m_externalLibraries.clear();
}

/*!
 * \brief Retrieves the external library model item.
 * \param index Item index to retrieve.
 * \return ExternalLibrary class instance.
 */
ExternalLibrary ExternalLibraryListModel::getExternalLibrary(const int index) const
{
    if (index < 0 || index > m_externalLibraries.count()) {
        return m_externalLibraries[0];
    } else {
        return  m_externalLibraries[index];
    }
}

/*!
 * \brief Adds a new external library to the list model.
 * \param externalLibrary ExternalLibrary class instance to add.
 */
void ExternalLibraryListModel::addExternalLibrary(ExternalLibrary externalLibrary)
{
    m_externalLibraries << externalLibrary;
    emit dataChanged(createIndex(0, 0), createIndex(m_externalLibraries.size() - 1, 0));
}

}
}
