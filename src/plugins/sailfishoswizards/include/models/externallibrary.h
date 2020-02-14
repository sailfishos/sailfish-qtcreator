/*
 * Qt Creator plugin with wizards of the Sailfish OS projects.
 * Copyright © 2020 FRUCT LLC.
 */

#ifndef EXTERNALLIBRARY_H
#define EXTERNALLIBRARY_H

#include <QStringList>

namespace SailfishOSWizards {
namespace Internal {

/*!
 * \brief ExternalLibrary class is a model for external library.
 * The class contains an information about the Qt external library.
 */
class ExternalLibrary
{

public:
    ExternalLibrary() {}

    void setName(const QString &name);
    void setLayoutName(const QString &layoutName);
    void setQtList(QStringList qtList);
    void setDependencyList(QStringList dependencyList);
    void setTypesList(QStringList types);
    void setYamlHeader(const QString &yamlHeader);
    void setSpecHeader(const QString &specHeader);

    QString getName() const;
    QString getLayoutName() const;
    QStringList getQtList() const;
    QStringList getDependencyList() const;
    QStringList getTypesList() const;
    QString getYamlHeader() const;
    QString getSpecHeader() const;

    void removeQtListItem(const int index);
    void removeDependencyListItem(const int index);

private:
    QString m_name;
    QString m_layoutName;
    QString m_yamlHeader;
    QString m_specHeader;
    QStringList m_qtList;
    QStringList m_dependencyList;
    QStringList m_types;
};

}
}

#endif // EXTERNALLIBRARY_H
