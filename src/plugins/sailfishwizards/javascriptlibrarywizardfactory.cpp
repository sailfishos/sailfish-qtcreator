/****************************************************************************
**
** Copyright (C) 2019 Open Mobile Platform LLC
** Contact: https://community.omprussia.ru/open-source
**
** Qt Creator class for creating a wizard for creating JS library.
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

#include "common.h"
#include "javascriptlibrarywizardfactory.h"
#include "fileinfopage.h"
#include "projectexplorer/projectwizardpage.h"
#include "sailfishwizardsconstants.h"
#include "sailfishprojectdependencies.h"

#include <QDebug>
#include <QFile>
#include <QStringList>
#include <QTextCodec>

namespace SailfishWizards {
namespace Internal {

/*!
 * \brief Constructs, sets the parameters of the wizard to create a JS library file.
 */
JavaScriptLibraryWizardFactory::JavaScriptLibraryWizardFactory()
{
    setId("JSLibraryWizard");
    setIconText(tr("JS Library"));
    setDisplayName(tr("JavaScript library"));
    setDescription(
        tr("The wizard allows to create JavaScript library files for Sailfish projects."));
    setCategory("Sailfish");
    setDisplayCategory("Sailfish");
    setFlags(IWizardFactory::WizardFlag::PlatformIndependent);
}

/*!
 * \brief Checks platform availability.
 * Returns \c true if the platform is available, otherwise \c false.
 * \param platformId platform identifier.
 */
bool JavaScriptLibraryWizardFactory::isAvailable(Id platformId) const
{
    Q_UNUSED(platformId);
    return true;
}

/*!
 * \brief Creates \с BaseFileWizard and sets the wizard pages.
 * Returns the \с BaseFileWizard with the specified pages.
 * \param parent The parent object instance.
 * \param parameters Contains an accessible set of parameters for the wizard.
 */
BaseFileWizard *JavaScriptLibraryWizardFactory::create(QWidget *parent,
                                                       const WizardDialogParameters &parameters) const
{
    BaseFileWizard *result = new BaseFileWizard(this, parameters.extraValues(), parent);
    result->addPage(new FileInfoPage(parameters.defaultPath()));
    QListIterator<QWizardPage *> extra_pages(result->extensionPages());
    while (extra_pages.hasNext()) {
        result->addPage(extra_pages.next());
    }
    result->setWindowTitle(displayName());
    return result;
}

/*!
 * \brief Sets the files to create.
 * Returns a set of files to create.
 * \param wizard Wizard for which files are created.
 * \param errorMessage The string to display the error message.
 * \return
 */
GeneratedFiles JavaScriptLibraryWizardFactory::generateFiles(const QWizard *wizard,
                                                             QString *errorMessage) const
{
    GeneratedFiles files;

    errorMessage->clear();
    GeneratedFile jsLibraryFile = generateJsFile(wizard, errorMessage);
    if (!errorMessage->isEmpty())
        return files;

    files << jsLibraryFile;

    GeneratedFile newQmldirFile;
    return files;
}

/*!
 * \brief Returns the suffix of the JS file.
 */
QString JavaScriptLibraryWizardFactory::javaScriptSuffix() const
{
    return preferredSuffix(SailfishWizards::Constants::JAVA_SCRIPT_MIMETYPE);
}

/*!
 * \brief Fills the template for the JS library file.
 * Returns a set of JS file to create.
 * \param wizard Wizard for which files are created.
 * \param errorMessage The string to display the error message.
 */
GeneratedFile JavaScriptLibraryWizardFactory::generateJsFile(const QWizard *wizard,
                                                             QString *errorMessage) const
{
    GeneratedFile jsLibraryFile;
    QHash <QString, QString> values;
    values.insert("pragma", wizard->field("pragma").toString());
    QString fileContents = Common::processTemplate("javascriptlibrary.js", values, errorMessage);
    if (!errorMessage->isEmpty())
        return jsLibraryFile;

    QString filePath = buildFileName(wizard->field("path").toString(), wizard->field("name").toString(),
                                     javaScriptSuffix());
    jsLibraryFile.setPath(filePath);
    jsLibraryFile.setContents(fileContents);
    return jsLibraryFile;
}

/*!
 * \brief Performs actions after creating the file.
 * Returns true if successful, otherwise false.
 * \param w wizard Wizard for which files are created.
 * \param l Contains information about the generated files.
 * \param errorMessage The string to display the error message.
 */
bool JavaScriptLibraryWizardFactory::postGenerateFiles(const QWizard *w, const GeneratedFiles &l,
                                                       QString *errorMessage) const
{
    Q_UNUSED(l);
    Q_UNUSED(errorMessage);
    QWizardPage *page =  w->currentPage();
    QString name = w->field("name").toString(), fullName;
    if (name.contains(".js")) {
        fullName = name.toLower();
        name = name.remove(name.length() - 3, 3);
    } else {
        fullName = name.toLower() + ".js";
    }
    Qmldir::addFileInQmldir(w->field("path").toString(), name, fullName, page);
    return true;
}

} // namespace Internal
} // namespace SailfishWizards
