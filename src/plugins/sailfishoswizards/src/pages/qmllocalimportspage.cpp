/*
 * Qt Creator plugin with wizards of the Sailfish OS projects.
 * Copyright © 2020 FRUCT LLC.
 */

#include "pages/qmllocalimportspage.h"

#include <QFileDialog>
#include <QMessageBox>

namespace SailfishOSWizards {
namespace Internal {

/*!
 * \brief Constructor for the local import page, sets up the page interface connections for buttons
 * to add and remove imports and registers a the 'LocalImports' field to use it inside
 * the QML-file template.
 */
QmlLocalImportsPage::QmlLocalImportsPage()
{
    m_pageUi.setupUi(this);
    QObject::connect(m_pageUi.addFileButton, &QPushButton::clicked,
                     this, &QmlLocalImportsPage::openFileDialog);
    QObject::connect(m_pageUi.removeButton, &QPushButton::clicked,
                     this, &QmlLocalImportsPage::removeSelectedImport);
    QObject::connect(m_pageUi.addDirButton, &QPushButton::clicked,
                     this, &QmlLocalImportsPage::openDirDialog);
    registerFieldWithName(QLatin1String("LocalImports"), this, "localImports");
}

/*!
 * \brief Initializes a target path to create QML file by retrieving the 'Path' value
 * from the wizard.
 */
void QmlLocalImportsPage::initializePage()
{
    m_wizard = qobject_cast<JsonWizard *>(wizard());
    m_path = m_wizard->stringValue(QLatin1String("Path"));
}

/*!
 * \brief Opens a dialog for the user to add js-files to the list of local imports.
 */
void QmlLocalImportsPage::openFileDialog()
{
    QStringList chosenFiles = QFileDialog::getOpenFileNames(
                            this, tr("Select one or more JavaScript files to import"),
                            m_path, tr("JavaScript files (*.js)"));
    if (!chosenFiles.isEmpty()) {
        addImports(chosenFiles);
    }
}

/*!
 * \brief Opens a dialog for the user to add directories to the list of local imports.
 */
void QmlLocalImportsPage::openDirDialog()
{
    QString chosenPath = QFileDialog::getExistingDirectory(
                this, tr("Select directory containing QML files to import"), m_path);
    if (chosenPath.compare(m_path) == 0) {
        QMessageBox message;
        message.setText(tr("You do not need to add the target directory to local imports, "
                           "because it is included by default."));
        message.setIcon(QMessageBox::Icon::Information);
        message.exec();
    } else if (!chosenPath.isEmpty()) {
        addImport(chosenPath);
    }
}

/*!
 * \brief Adds the given list of new imports to the list of already added imports.
 * \param imports List of imports to add.
 */
void QmlLocalImportsPage::addImports(QStringList imports)
{
    for (QString import : imports) {
        QString relativePath = QDir(m_path).relativeFilePath(import);
        if (!m_importsList.contains(relativePath))
            m_importsList.append(relativePath);
    }
    m_importsList.sort();
    m_pageUi.listImports->clear();
    m_pageUi.listImports->addItems(m_importsList);
}

/*!
 * \brief Adds a new import to the list of already added imports.
 * \param import New import to add.
 */
void QmlLocalImportsPage::addImport(const QString &import)
{
    addImports(QStringList() << import);
}

/*!
 * \brief Deletes the current selected local import.
 */
void QmlLocalImportsPage::removeSelectedImport()
{
    m_importsList.removeAt(m_pageUi.listImports->currentRow());
    m_pageUi.listImports->takeItem(m_pageUi.listImports->currentRow());
}

/*!
 * Creates and returns a string with list of imports for the new QML-file.
 * Resulting string contains list of substrings in format 'import <path>\n'.
 * \return String with imports for the QML-file.
 */
QString QmlLocalImportsPage::localImports() const
{
    QString localImportsString;
    for (QString import : m_importsList) {
        localImportsString.append(QString("import \"%1\"\n").arg(import));
    }
    return localImportsString;
}

}
}
