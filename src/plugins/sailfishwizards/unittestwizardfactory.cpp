/****************************************************************************
**
** Copyright (C) 2019 Open Mobile Platform LLC
** Contact: https://community.omprussia.ru/open-source
**
** Qt Creator class for creating a wizard for creating unit test files.
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

#include "unittestwizardfactory.h"
#include "sailfishwizardsconstants.h"
#include "common.h"
#include "projectexplorer/projectwizardpage.h"
#include "projectexplorer/projectnodes.h"
#include "spectaclefilewizardpages.h"
#include "unittestwizardpages.h"

#include <QDir>
#include <QFileInfo>
#include <QScreen>
#include <QTextCodec>
#include <QGuiApplication>
#include <QString>

namespace SailfishWizards {
namespace Internal {
/*!
 * \brief Constructs, sets the parameters of the wizard to create a unit test files.
 */
UnitTestWizardFactory::UnitTestWizardFactory()
{
    setId("UnitTestWizard");
    setIconText(tr("C++"));
    setDisplayName(tr("Unit test for C++ class"));
    setDescription(
        tr("The wizard allows to create the unit test for the C++ class in the Sailfish project."));
    setCategory("Sailfish");
    setDisplayCategory("Sailfish");
    setFlags(IWizardFactory::WizardFlag::PlatformIndependent);
}

/*!
 * \brief Platform availability.
 * Returns \c true if the platform is available, otherwise \c false.
 * \param platformId platform identifier.
 */
bool UnitTestWizardFactory::isAvailable(Id platformId) const
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
BaseFileWizard *UnitTestWizardFactory::create(QWidget *parent,
                                              const WizardDialogParameters &parameters) const
{
    BaseFileWizard *result = new BaseFileWizard(this, parameters.extraValues(), parent);
    result->addPage(new LocationPageUnitTest(parameters.defaultPath()));
    QListIterator<QWizardPage *> extra_pages(result->extensionPages());
    while (extra_pages.hasNext()) {
        result->addPage(extra_pages.next());
    }
    QRect window = QGuiApplication::primaryScreen()->geometry();
    result->setMinimumSize(static_cast<int>(window.width() * 0.4),
                           static_cast<int>(window.height() * 0.4));
    result->setWindowTitle(displayName());
    return result;
}

/*!
 * \brief Sets the files to create.
 * Returns a set of files to create.
 * \param wizard Wizard for which files are created.
 * \param errorMessage The string to display the error message.
 */
GeneratedFiles UnitTestWizardFactory::generateFiles(const QWizard *wizard,
                                                    QString *errorMessage) const
{
    GeneratedFiles files;
    errorMessage->clear();
    GeneratedFile hFile = generateHFile(wizard, errorMessage);
    GeneratedFile cppFile = generateCppFile(wizard, errorMessage);
    if (!errorMessage->isEmpty()) {
        return files;
    }
    files << hFile;
    files << cppFile;
    return files;
}

/*!
 * \brief Sets the header file to create.
 * Returns a set of header file to create.
 * \param wizard Wizard for which files are created.
 * \param errorMessage The string to display the error message.
 */
GeneratedFile UnitTestWizardFactory::generateHFile(const QWizard *wizard,
                                                   QString *errorMessage) const
{
    GeneratedFile hFile;
    QDir projectDir(SpectacleFileSelectionPage::findProjectRoot(wizard->field("path").toString()));
    QHash <QString, QString> values;
    values.insert("nameClass", wizard->field("nameClass").toString());
    values.insert("nameClassUpper", wizard->field("nameClass").toString().toUpper());
    values.insert("nameH", wizard->field("nameH").toString());
    values.insert("selectHeaderFile",
                  projectDir.relativeFilePath((wizard->field("selectHeaderFile").toString())));
    QString fileContents = Common::processTemplate("unittestfileheader.template", values, errorMessage);
    if (!errorMessage->isEmpty()) {
        return hFile;
    }
    QString filePath = buildFileName(wizard->field("path").toString(),
                                     wizard->field("nameH").toString().toLower(), hSuffix());
    hFile.setPath(filePath);
    hFile.setContents(fileContents);
    return hFile;
}

/*!
 * \brief Sets the source file to create.
 * Returns a set of source file to create.
 * \param wizard Wizard for which files are created.
 * \param errorMessage The string to display the error message.
 */
GeneratedFile UnitTestWizardFactory::generateCppFile(const QWizard *wizard,
                                                     QString *errorMessage) const
{
    GeneratedFile cppFile;
    QHash <QString, QString> values;
    values.insert("nameClass", wizard->field("nameClass").toString());
    values.insert("nameH", wizard->field("nameH").toString());
    QString fileContents = Common::processTemplate("unittestfilesource.template", values, errorMessage);
    if (!errorMessage->isEmpty()) {
        return cppFile;
    }
    QString filePath = buildFileName(wizard->field("path").toString(),
                                     wizard->field("nameCpp").toString().toLower(), cppSuffix());
    cppFile.setPath(filePath);
    cppFile.setContents(fileContents);
    return cppFile;
}

/*!
 * \brief Returns the mime type header file.
 * \return
 */
QString UnitTestWizardFactory::hSuffix() const
{
    return preferredSuffix(SailfishWizards::Constants::CPP_HEADER_MYMETYPE);
}

/*!
 * \brief Returns the mime type source file.
 * \return
 */
QString UnitTestWizardFactory::cppSuffix() const
{
    return preferredSuffix(SailfishWizards::Constants::CPP_SOURCE_MYMETYPE);
}

} // namespace Internal
} // namespace SailfishWizards
