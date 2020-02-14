/*
 * Qt Creator plugin with wizards of the Sailfish OS projects.
 * Copyright © 2020 FRUCT LLC.
 */

#ifndef EXTERNALLIBRARYLISTMODEL_H
#define EXTERNALLIBRARYLISTMODEL_H

#include <QAbstractListModel>

#include "externallibrary.h"

namespace SailfishOSWizards {
namespace Internal {

/*!
 * \brief ExternalLibraryListModel class is a list model of external libraries.
 */
class ExternalLibraryListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum ExternalLibraryRoles {
        LayoutName = Qt::DisplayRole,
        Name = Qt::UserRole + 1,
        QtList = Qt::UserRole + 2,
        YamlHeader = Qt::UserRole + 3,
        SpecHeader = Qt::UserRole + 4,
        DependencyList = Qt::UserRole + 5,
        Types = Qt::UserRole + 6
    };

    explicit ExternalLibraryListModel(QObject *parent = nullptr);

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
    int rowCount(const QModelIndex &parent) const Q_DECL_OVERRIDE;
    bool removeRows(int pos, int count, const QModelIndex &parent = QModelIndex()) Q_DECL_OVERRIDE;

    int size() const;
    void clear();

    ExternalLibrary getExternalLibrary(const int index) const;
    void addExternalLibrary(ExternalLibrary externalLibrary);

private:
    QList<ExternalLibrary> m_externalLibraries;
    QHash<int, QString> m_roles;
};

}
}

#endif // EXTERNALLIBRARYLISTMODEL_H
