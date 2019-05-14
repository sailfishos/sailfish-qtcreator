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

#include "checkcomponenttablemodel.h"

namespace SailfishWizards {
namespace Internal {

/*!
 * \brief Returns the number of rows of the model.
 * \param parent The parent object index.
 */
int CheckComponentTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_components.count();
}

/*!
 * \brief Returns the number of columns of the model.
 * \param parent The parent object instance.
 */
int CheckComponentTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

/*!
 * \brief Returns the field of the components table
 * by the specified parameter.
 * \param index Item index.
 * \param role Role number.
 */
QVariant CheckComponentTableModel::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    if (role == Qt::DisplayRole) {
        return m_components.value(row).getName();
    }
    if (role == Qt::CheckStateRole) {
        if (m_checks.value(row))
            return Qt::Checked;
        else return Qt::Unchecked;
    }
    return QVariant();
}

/*!
 * \brief Returns the flags of model item.
 * \param index Item index.
 */
Qt::ItemFlags CheckComponentTableModel::flags(const QModelIndex &index) const
{
    Q_UNUSED(index);
    return Qt::ItemIsUserCheckable | Qt::ItemIsEnabled;
}

/*!
 * \brief Checks or removes check from the component checked by user.
 * \param index Item index.
 * \param value Object containing information about user's action.
 * \param role Role number.
 */
bool CheckComponentTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (index.isValid()) {
        if (role == Qt::CheckStateRole) {
            m_checks[index.row()] = value.toBool();
            emit dataChanged(index, index);
            return true;
        }
    }
    return false;
}

/*!
 * \brief Removes checks from all of the components.
 */
void CheckComponentTableModel::clearChecks()
{
    for (int i = 0; i < m_checks.count(); ++i) {
        m_checks[i] = false;
        emit dataChanged(createIndex(i, 0), createIndex(i, 0));
    }
}

/*!
 * \brief Checks or removes check from the component by its name.
 * \param name Name of the component.
 * \param check The value needed to set.
 */
void CheckComponentTableModel::checkByName(QString name, bool check)
{
    if (m_components.contains(ProjectComponent(name)))
        setData(createIndex(m_components.indexOf(ProjectComponent(name)), 0), QVariant(check),
                Qt::CheckStateRole);
}

QList<bool> CheckComponentTableModel::getChecks() const
{
    return m_checks;
}

} // namespace Internal
} // namespace SailfishWizards
