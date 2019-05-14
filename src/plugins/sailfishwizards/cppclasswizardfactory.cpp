/****************************************************************************
**
** Copyright (C) 2019 Open Mobile Platform LLC
** Contact: https://community.omprussia.ru/open-source
**
** Qt Creator class for creating a wizard for creating C++ files.
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

#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QFile>
#include <QStringList>
#include <QScreen>
#include <QTextCodec>

#include "common.h"
#include "cppclasswizardfactory.h"
#include "cppclasswizardpages.h"
#include "projectexplorer/projectwizardpage.h"
#include "sailfishwizardsconstants.h"
#include "sailfishprojectdependencies.h"
#include "wizardpage.h"

namespace SailfishWizards {
namespace Internal {

/*!
 * \brief Constructs, sets the parameters of the wizard to create a C++ file.
 */
CppClassWizardFactory::CppClassWizardFactory()
{
    setId("C++ClassWizard");
    setIconText(tr("C++"));
    setDisplayName(tr("C++ сlass"));
    setDescription(tr("The wizard allows to create C++ class files for the Sailfish projects."));
    setCategory("Sailfish");
    setDisplayCategory("Sailfish");
    setFlags(IWizardFactory::WizardFlag::PlatformIndependent);
}

/*!
 * \brief Loads external libraries data from the resource json file.
 * Returns model containing loaded libraries data.
 * \sa DependencyModel
 */
DependencyModel *CppClassWizardFactory::loadAllExternalLibraries() const
{
    DependencyModel *model = new DependencyModel;
    QRegExp rx(" ");
    QString value;
    ExternalLibrary newExternalLibrary;
    QStringList strList;
    QJsonArray jsonArray;
    QFile jsonFile(":/sailfishwizards/cppwizard/data/cpp-modules.json");
    jsonFile.open(QIODevice::ReadOnly | QIODevice::Text);
    value += QTextCodec::codecForName("UTF-8")->toUnicode(jsonFile.readAll());
    jsonFile.close();
    QJsonDocument jsonDoc = QJsonDocument::fromJson(value.toUtf8());
    QJsonObject jsonObject = jsonDoc.object();
    for (QJsonObject::Iterator iter = jsonObject.begin(); iter != jsonObject.end(); ++iter) {
        newExternalLibrary.setName(iter.key());
        QJsonObject val = iter.value().toObject();

        newExternalLibrary.setLayoutName(val["layoutname"].toString());

        jsonArray = val["qt"].toArray();
        for (int i = 0; i < jsonArray.count(); i++)
            strList << jsonArray[i].toString();

        newExternalLibrary.setQtList(strList);
        strList.clear();

        newExternalLibrary.setYamlHeader(val["yamlheader"].toString());
        newExternalLibrary.setSpecHeader(val["specheader"].toString());

        jsonArray = val["dependencies"].toArray();
        for (int i = 0; i < jsonArray.count(); i++)
            strList << jsonArray[i].toString().split(rx).value(0);

        newExternalLibrary.setDependencyList(strList);
        strList.clear();

        jsonArray = val["classes"].toArray();
        for (int i = 0; i < jsonArray.count(); i++)
            strList << jsonArray[i].toString();

        newExternalLibrary.setTypesList(strList);
        strList.clear();
        model->addExternalLibrary(newExternalLibrary);
    }
    return model;
}

/*!
 * \brief Checks platform availability.
 * Returns \c true if the platform is available, otherwise \c false.
 * \param platformId platform identifier.
 */
bool CppClassWizardFactory::isAvailable(Id platformId) const
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
BaseFileWizard *CppClassWizardFactory::create(QWidget *parent,
                                              const WizardDialogParameters &parameters) const
{
    DependencyModel *allExternalLibraryList = loadAllExternalLibraries();
    DependencyModel *selectExternalLibraryList = new DependencyModel;
    BaseFileWizard *result = new BaseFileWizard(this, parameters.extraValues(), parent);
    result->addPage(new LocationPage(parameters.defaultPath()));
    result->addPage(new ExternalLibraryPage(allExternalLibraryList, selectExternalLibraryList));
    result->addPage(new OptionPage(selectExternalLibraryList));
    QListIterator<QWizardPage *> extra_pages(result->extensionPages());
    while (extra_pages.hasNext())
        result->addPage(extra_pages.next());

    QRect window = QGuiApplication::primaryScreen()->geometry();
    result->setMinimumSize(static_cast<int>(window.width() * 0.4),
                           static_cast<int>(window.height() * 0.4));
    delete allExternalLibraryList;
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
GeneratedFiles CppClassWizardFactory::generateFiles(const QWizard *wizard,
                                                    QString *errorMessage) const
{
    GeneratedFiles files;
    QString nameClass;
    QStringList nameSpaceClass;
    parseNameClass(wizard->field("nameClass").toString(), nameClass, nameSpaceClass);
    errorMessage->clear();
    GeneratedFile hFile = generateHFile(wizard, errorMessage, nameClass, nameSpaceClass);
    GeneratedFile cppFile = generateCppFile(wizard, errorMessage, nameClass, nameSpaceClass);
    if (!errorMessage->isEmpty())
        return files;

    files << hFile;
    files << cppFile;
    return files;
}
/*!
 * \brief Fills the template for the header file.
 * Returns a set of header file to create.
 * \param wizard Wizard for which files are created.
 * \param errorMessage The string to display the error message.
 * \param nameClass The name of the generated class.
 * \param nameSpaceClass Namespaces that contain the generated class.
 */
GeneratedFile CppClassWizardFactory::generateHFile(const QWizard *wizard,
                                                   QString *errorMessage, const QString &nameClass, const QStringList &nameSpaceClass) const
{
    GeneratedFile hFile;
    QString includs = "", nameSpaceList = "", nameSpaceClose = "";
    if (wizard->field("baseClass").toString() != "QObject")
        includs += "#include <QObject>\n";
    QHash <QString, QString> values;
    if (!wizard->field("baseClass").toString().isEmpty())
        values.insert("baseClassFlag", "true");
    else
        values.insert("baseClassFlag", "false");

    if (nameSpaceClass.size() != 0) {
        nameSpaceList += "\n";
        nameSpaceClose += "\n";
        for (QString nameSpace : nameSpaceClass) {
            nameSpaceClose += "}\n";
            nameSpaceList += "namespace " + nameSpace + " {\n";
        }
        nameSpaceList += "\n";
        nameSpaceClose += "\n";
    }

    values.insert("nameSpaceList", nameSpaceList);
    values.insert("nameSpaceClose", nameSpaceClose);
    values.insert("includs", includs);
    values.insert("nameClass", nameClass);
    values.insert("nameClassUpper", nameClass.toUpper());
    values.insert("nameH", wizard->field("nameH").toString());
    values.insert("baseClass", wizard->field("baseClass").toString());
    QString fileContents = Common::processTemplate("cppclassfileheader.template", values, errorMessage);
    if (!errorMessage->isEmpty())
        return hFile;

    QString filePath = buildFileName(wizard->field("path").toString(),
                                     wizard->field("nameH").toString().toLower(), hSuffix());
    hFile.setPath(filePath);
    hFile.setContents(fileContents);
    return hFile;
}

/*!
 * \brief Fills the template for the source file.
 * Returns a set of source file to create.
 * \param wizard Wizard for which files are created.
 * \param errorMessage The string to display the error message.
 * \param nameClass The name of the generated class.
 * \param nameSpaceClass Namespaces that contain the generated class.
 */
GeneratedFile CppClassWizardFactory::generateCppFile(const QWizard *wizard,
                                                     QString *errorMessage, const QString &nameClass, const QStringList &nameSpaceClass) const
{
    GeneratedFile cppFile;
    QHash <QString, QString> values;
    QString nameSpaceList = "", nameSpaceClose = "";
    if (!wizard->field("baseClass").toString().isEmpty())
        values.insert("baseClassFlag", "true");
    else
        values.insert("baseClassFlag", "false");

    if (nameSpaceClass.size() != 0) {
        nameSpaceList += "\n";
        nameSpaceClose += "\n";
        for (QString nameSpace : nameSpaceClass) {
            nameSpaceClose += "}\n";
            nameSpaceList += "namespace " + nameSpace + " {\n";
        }
        nameSpaceList += "\n";
        nameSpaceClose += "\n";
    }

    values.insert("nameSpaceList", nameSpaceList);
    values.insert("nameSpaceClose", nameSpaceClose);
    values.insert("nameClass", nameClass);
    values.insert("nameH", wizard->field("nameH").toString());
    values.insert("baseClass", wizard->field("baseClass").toString());
    QString fileContents = Common::processTemplate("cppclassfilesource.template", values, errorMessage);
    if (!errorMessage->isEmpty())
        return cppFile;

    QString filePath = buildFileName(wizard->field("path").toString(),
                                     wizard->field("nameCpp").toString().toLower(), cppSuffix());
    cppFile.setPath(filePath);
    cppFile.setContents(fileContents);
    return cppFile;
}

/*!
 * \brief Returns the mime type header file.
 */
QString CppClassWizardFactory::hSuffix() const
{
    return preferredSuffix(SailfishWizards::Constants::CPP_HEADER_MYMETYPE);
}

/*!
 * \brief Returns the mime type source file.
 */
QString CppClassWizardFactory::cppSuffix() const
{
    return preferredSuffix(SailfishWizards::Constants::CPP_SOURCE_MYMETYPE);
}

/*!
 * \brief Performs actions after creating the file.
 * Returns true if successful, otherwise false.
 * \param w wizard Wizard for which files are created.
 * \param l Contains information about the generated files.
 * \param errorMessage The string to display the error message.
 */
bool CppClassWizardFactory::postGenerateFiles(const QWizard *w, const GeneratedFiles &l,
                                              QString *errorMessage) const
{
    Q_UNUSED(l);
    Q_UNUSED(errorMessage);
    DependencyModel *selectExternalLibraryList = static_cast<OptionPage *>(w->page(
                                                                               PageId::OPTIONS))->getModulList();
    if (selectExternalLibraryList->size() != 0) {
        QFileInfo proFileInfo;
        QString filePath, path;

        const LocationPage *locationPage = static_cast<const LocationPage *>(w->page(PageId::LOCATION));
        if (locationPage) {
            const QString proFilePath = Common::getProFilePath(locationPage->getDefaultPath());
            proFileInfo.setFile(proFilePath);
        }

        if (proFileInfo.exists()) {
            filePath = proFileInfo.path();
            path = proFileInfo.filePath();
        } else {
            filePath = w->field("path").toString();
            path = filePath;
        }

        QWizardPage *page = w->currentPage();
        SailfishProjectDependencies::addDependecyInYamlFile(filePath, selectExternalLibraryList, page);
        SailfishProjectDependencies::addDependencyInProFile(path, selectExternalLibraryList, page);
    }
    return true;
}

/*!
 * \brief Parses the class name,
 * allocates namespace from it.
 * \param nameClass Сlass name.
 * \param outNameClass Outer parameter for storing name of created class.
 * \param outNameSpaceClass Outer parameter for storing the namespaces of created class.
 */
void CppClassWizardFactory::parseNameClass(const QString &nameClass, QString &outNameClass,
                                           QStringList &outNameSpaceClass) const
{
    QRegExp rx("::");
    if (nameClass.contains("::")) {
        outNameSpaceClass = nameClass.split(rx);
        outNameClass = outNameSpaceClass.takeLast();
    } else {
        outNameClass = nameClass;
    }
}

} // namespace Internal
} // namespace SailfishWizards
