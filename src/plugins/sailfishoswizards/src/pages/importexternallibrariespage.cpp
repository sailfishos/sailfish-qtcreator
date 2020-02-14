/*
 * Qt Creator plugin with wizards of the Sailfish OS projects.
 * Copyright © 2020 FRUCT LLC.
 */

#include "pages/importexternallibrariespage.h"

#include "sailfishoswizards/utils.h"

#include <QJsonDocument>
#include <QJsonArray>

namespace SailfishOSWizards {
namespace Internal {

/*!
 * \brief Constructor initializes the class fields, loads  descriptions of the external libraries
 * available to import and sets up connections for the page and dialog buttons.
 */
ImportExternalLibrariesPage::ImportExternalLibrariesPage()
    : m_selectedLibraries(new ExternalLibraryListModel)
{
    m_pageUi.setupUi(this);
    m_selectLibrariesDialogUi.setupUi(&m_selectLibrariesDialog);
    m_allLibraries = Utils::loadExternalLibraries(":/sailfishoswizards/data/qml-modules.json");
    m_selectLibrariesDialogUi.itemList->setModel(m_allLibraries);
    m_pageUi.libraryList->setModel(m_selectedLibraries);
    connect(m_pageUi.addButton, &QPushButton::clicked,
            this, &ImportExternalLibrariesPage::openSelectLibrariesDialog);
    connect(m_pageUi.removeButton, &QPushButton::clicked,
            this, &ImportExternalLibrariesPage::removeSelectedLibraryFromList);
    connect(m_selectLibrariesDialogUi.cancelButton, &QPushButton::clicked,
            &m_selectLibrariesDialog, &QDialog::close);
    connect(m_selectLibrariesDialogUi.itemList, &QListView::doubleClicked,
            this, &ImportExternalLibrariesPage::addSelectedLibrary);
    connect(m_selectLibrariesDialogUi.addButton, &QPushButton::clicked,
            this, &ImportExternalLibrariesPage::addMultimpleSelectedLibraries);
    registerFieldWithName(QLatin1String("QmlLibrariesImports"), this, "qmlLibrariesImports");
    registerFieldWithName(QLatin1String("QmlLibrariesTypes"), this, "qmlLibrariesTypes");
}

/*!
 * \brief Creates and returns a string with list of imports for the new QML-file.
 * Resulting string contains list of substrings in format 'import <lib-name-with-version>\n'.
 * \return String with imports for the QML-file.
 */
QString ImportExternalLibrariesPage::qmlLibrariesImports() const
{
    QString importsString;
    for (int i = 0; i < m_selectedLibraries->size(); i++) {
        QString import = m_selectedLibraries->index(i, 0).data(ExternalLibraryListModel::Name).toString();
        importsString.append(QString("import %1\n").arg(import));
    }
    return importsString;
}

/*!
 * \brief Creates and returns a string with QML-types from selected external libraries.
 * \return QString with names of types that can be used as parent for new QML-type
 * in the JSON-array format.
 */
QString ImportExternalLibrariesPage::qmlLibrariesTypes() const
{
    QStringList types = QStringList() << "Item";
    for (int i = 0; i < m_selectedLibraries->size(); i++) {
        types << m_selectedLibraries->index(i, 0).data(ExternalLibraryListModel::Types).toStringList();
    }
    QJsonDocument doc;
    doc.setArray(QJsonArray::fromStringList(types));
    return QString::fromUtf8(doc.toJson());
}

/*!
 * \brief Opens a dialog to select external libraries.
 */
void ImportExternalLibrariesPage::openSelectLibrariesDialog()
{
    m_selectLibrariesDialog.exec();
}

/*!
 * \brief Removes the selected library from the selected list.
 */
void ImportExternalLibrariesPage::removeSelectedLibraryFromList()
{
    int index = m_pageUi.libraryList->currentIndex().row();
    if (index < 0 || index > m_selectedLibraries->size())
        return;
    ExternalLibrary newExternalLibrary = m_selectedLibraries->getExternalLibrary(index);
    m_allLibraries->addExternalLibrary(newExternalLibrary);
    m_selectedLibraries->removeRows(index, 1);
}

/*!
 * \brief Adds the selected library from the 'All libraries' list by its QModelIndex item.
 * \param index QModelIndex instance.
 */
void ImportExternalLibrariesPage::addSelectedLibrary(const QModelIndex &index)
{
    addExternalLibrary(index.row());
    m_allLibraries->removeRows(index.row(), 1);
    m_selectLibrariesDialog.close();
}

/*!
 * \brief Adds the multiple selected libraries from the 'All libraries' to the list of selected.
 */
void ImportExternalLibrariesPage::addMultimpleSelectedLibraries()
{
    QModelIndexList indexes = m_selectLibrariesDialogUi.itemList->selectionModel()->selectedIndexes();
    std::sort(indexes.begin(), indexes.end());
    for (QModelIndex index : indexes) {
        addExternalLibrary(index.row());
    }
    for (int i = indexes.count() - 1; i >= 0; i--) {
        m_allLibraries->removeRows(indexes.value(i).row(), 1);
    }
    m_selectLibrariesDialog.close();
}

/*!
 * \brief Adds the library to the list of selected by its index.
 * \param index Integer value with index of library inside 'All libraries' list.
 */
void ImportExternalLibrariesPage::addExternalLibrary(const int index)
{
    if (index < 0 || index > m_allLibraries->size())
        return;
    m_selectedLibraries->addExternalLibrary(m_allLibraries->getExternalLibrary(index));
}

}
}
