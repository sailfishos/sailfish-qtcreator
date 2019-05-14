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

#include <QObject>
#include <QAbstractTableModel>
#include "dependencylistmodel.h"

namespace SailfishWizards {
namespace Internal {
/*!
 * \brief The CheckComponentTableModel class defines the model of the components table for the spectacle project component page.
 * \sa QAbstractTableModel, SpectacleFileProjectTypePage
 */
class CheckComponentTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    CheckComponentTableModel(QList<ProjectComponent> components, QList<bool> &checks,
                             QObject *parent = nullptr)
        : QAbstractTableModel(parent), m_components(components), m_checks(checks) {}
    int rowCount(const QModelIndex &parent = QModelIndex()) const ;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
    Qt::ItemFlags flags(const QModelIndex &index) const ;
    void checkByName(QString name, bool check);
    void clearChecks();
    /*!
     * \brief Returns check list of components.
     */
    QList<bool> getChecks() const;

private:
    QList<ProjectComponent> m_components;
    QList<bool> m_checks;
};
} // namespace Internal
} // namespace SailfishWizards
