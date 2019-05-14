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

#include "dbusservice.h"
#include <QObject>
#include <QAbstractTableModel>

namespace SailfishWizards {
namespace Internal {

/*!
 * \brief Class for the list of DBus service's methods.
 */
class DBusServicesTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:

    /*!
     * \brief Constructor initializes class's fields.
     * \param services List of services with methods from the selected interface.
     * \param checks List of checks from selected methods.
     * \param parent Parent of the class instance.
     */
    DBusServicesTableModel(const QList<DBusService> &services, const QList<bool> &checks, bool isMethodsTable,
                           bool isAdvancedMode, QObject *parent = nullptr)
        : QAbstractTableModel(parent)
        , m_services(services)
        , m_checks(checks)
        , m_isMethodsTable(isMethodsTable)
        , m_isAdvancedMode(isAdvancedMode)
    {}

    int rowCount(const QModelIndex &parent = QModelIndex()) const ;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
    Qt::ItemFlags flags(const QModelIndex &index) const ;
    void clearTable();
    void update(const QList<DBusService> &collection, bool isMethodsTable);

    /*!
     * \brief Return list of checks.
     */
    QList<bool> getChecks() const
    {
        return m_checks;
    }

    /*!
     * \brief Return table status.
     */
    bool getIsMethodsTable() const
    {
        return m_isMethodsTable;
    }

    /*!
     * \brief Return list of services with methods from the selected interface.
     */
    QList<DBusService> getServices() const
    {
        return m_services;
    }

    /*!
     * \brief Sets the advanced mode status.
     */
    void setAdvancedMode(bool advancedMode)
    {
        m_isAdvancedMode = advancedMode;
    }

    /*!
     * \brief Sets the list of checks.
     */
    void setChecks(const QList<bool> &checks)
    {
        m_checks = checks;
    }

    /*!
     * \brief Sets the table status.
     */
    void setMethodsTableStatus(bool isMethodsTable)
    {
        m_isMethodsTable = isMethodsTable;
    }

    /*!
     * \brief Sets the list of services with methods from the selected interface.
     */
    void setServices(const QList<DBusService> &services)
    {
        m_services = services;
    }
private:
    QList<DBusService> m_services;
    QList<bool> m_checks;
    bool m_isMethodsTable;
    bool m_isAdvancedMode;
signals:
    void checksChanged();
};

} // namespace Internal
} // namespace SailfishWizards
