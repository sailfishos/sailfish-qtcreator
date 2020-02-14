/*
 * Qt Creator plugin with wizards of the Sailfish OS projects.
 * Copyright © 2020 FRUCT LLC.
 */

#include "models/externallibrary.h"

namespace SailfishOSWizards {
namespace Internal {

/*!
 * \brief Sets the name for the external library.
 * \param name New name.
 */
void ExternalLibrary::setName(const QString &name)
{
    m_name = name;
}

/*!
 * \brief Sets the visible name for the external library.
 * \param layoutName New layout name.
 */
void ExternalLibrary::setLayoutName(const QString &layoutName)
{
    m_layoutName = layoutName;
}

/*!
 * \brief Sets the connection list for a .pro-file
 * \param qtList New list.
 */
void ExternalLibrary::setQtList(QStringList qtList)
{
    m_qtList = qtList;
}

/*!
 * \brief Sets the list of dependencies.
 * \param dependencyList New dependecy list.
 */
void ExternalLibrary::setDependencyList(QStringList dependencyList)
{
    m_dependencyList = dependencyList;
}

/*!
 * \brief Sets the list of basic types.
 * \param types New types.
 */
void ExternalLibrary::setTypesList(QStringList types)
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
 * \brief Removes the dependency list entry for the .pro file.
 * \param index Item index.
 */
void ExternalLibrary::removeQtListItem(const int index)
{
    m_qtList.removeAt(index);
}

/*!
 * \brief Removes the dependency list.
 * \param index Item index.
 */
void ExternalLibrary::removeDependencyListItem(const int index)
{
    m_dependencyList.removeAt(index);
}

}
}
