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

#include "dbusservicestablemodel.h"

namespace SailfishWizards {
namespace Internal {

/*!
 * \brief Clears the methods table.
 */
void DBusServicesTableModel::clearTable()
{
    for (DBusService service : m_services) {
        beginRemoveRows(QModelIndex(), m_services.indexOf(service), m_services.indexOf(service));
        m_services.removeOne(service);
        endRemoveRows();
    }
    m_checks.clear();
    emit dataChanged(QModelIndex(), QModelIndex());
}

/*!
 * \brief Clears the methods table and fill with new data from \c collection.
 * \param collection New list of services with methods from the selected interface.
 */
void DBusServicesTableModel::update(const QList<DBusService> &collection, bool isMethodsTable)
{
    for (DBusService service : m_services) {
        beginRemoveRows(QModelIndex(), m_services.indexOf(service), m_services.indexOf(service));
        m_services.removeOne(service);
        endRemoveRows();
    }
    m_checks.clear();
    for (const DBusService &service : collection) {
        beginInsertRows(QModelIndex(), collection.indexOf(service), collection.indexOf(service));
        if (isMethodsTable)
            service.getMethod();
        else
            service.getSignal();
        m_checks.append(false);
        endInsertRows();
    }
    setServices(collection);
    emit dataChanged(createIndex(0, 0), createIndex(collection.size() - 1, 1));
}

/*!
 * \brief Return the number of methods table rows.
 * \param parent Item index of parent.
 */
int DBusServicesTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_services.count();
}

/*!
 * \brief Return the number of methods table columns.
 * \param parent Item index of parent.
 */
int DBusServicesTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

/*!
 * \brief Returns the field of the DBus service's
 * method with check by the specified parameter.
 * \param index Item index.
 * \param role Role number.
 */
QVariant DBusServicesTableModel::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    if (role == Qt::DisplayRole) {
        if (m_isMethodsTable && !m_isAdvancedMode) {
            QString retStr = m_services.value(row).getInterface() + ": " + m_services.value(row).getMethod();
            return retStr;
        } else if (!m_isMethodsTable && !m_isAdvancedMode) {
            QString retStr = m_services.value(row).getInterface() + ": " + m_services.value(row).getSignal();
            return retStr;
        } else if (m_isMethodsTable) {
            QString retStr = m_services.value(row).getMethod();
            return retStr;
        } else {
            QString retStr = m_services.value(row).getSignal();
            return retStr;
        }
    }
    if (role == Qt::CheckStateRole) {
        if (m_checks.value(row))
            return Qt::Checked;
        else return Qt::Unchecked;
    }
    return QVariant();
}

/*!
 * \brief Return the flags of model.
 * \param index Item index.
 */
Qt::ItemFlags DBusServicesTableModel::flags(const QModelIndex &index) const
{
    Q_UNUSED(index);
    return Qt::ItemIsEnabled;
}

/*!
 * \brief Set selected method.
 * \param index Item index.
 * \param value Item value.
 * \param role Role number.
 */
bool DBusServicesTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (index.isValid()) {
        if (role == Qt::CheckStateRole) {
            m_checks[index.row()] = value.toBool();
            emit dataChanged(index, index);
            emit checksChanged();
            return true;
        }
    }
    return false;
}

} // namespace Internal
} // namespace SailfishWizards
