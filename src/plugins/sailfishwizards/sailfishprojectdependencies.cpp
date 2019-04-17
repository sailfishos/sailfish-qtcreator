/****************************************************************************
**
** Copyright (C) 2019 Open Mobile Platform LLC
** Contact: https://community.omprussia.ru/open-source
**
** Qt Creator modul for adding dependencies to file.
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

#include "sailfishprojectdependencies.h"
#include "wizardpage.h"

#include <QFile>
#include <QDir>
#include <QTextCodec>
#include "sailfishprojectdependencies.h"
#include "wizardpage.h"

#include <QFile>
#include <QDir>
#include <QTextCodec>

namespace SailfishWizards {
namespace Internal {

namespace YamlFileHandler {
QString locateYamlFile(QString dir, QWidget *page);
QStringList readYamlFile(QString path, DependencyModel *selectExternalLibraryList);
QStringList parseYamlFile(QStringList strListYaml,  DependencyModel *selectExternalLibraryList);
QStringList addListOfDependencyYaml(QStringList dependecyList, QStringList strListYaml,
                                    int lineNumberWithDependencies, int lineNumberWithHeading);
void writeYamlFile(QStringList strListYaml, QString path);
} // namespace HandlerYamlFile

namespace SpecFileHandler {
QString locateSpecFile(QString dir, QWidget *page);
QStringList readSpecFile(QString path, DependencyModel *selectExternalLibraryList);
QStringList parseSpecFile(QStringList strListSpec,  DependencyModel *selectExternalLibraryList);
QStringList addListOfDependencySpec(QStringList dependecyList,  QString specHeader, QString yamlHeader,
                                    QStringList strListSpec, int lineNumberWithDependencies, int lineNumberWithHeading);
void writeSpecFile(QStringList strListSpec, QString path);
} // namespece HandlerSpecFile

namespace ProFileHandler  {
QString locateProFile(QString path, QWidget *page);
QStringList readProFile(QString path, DependencyModel *selectExternalLibraryList);
QStringList parseProFile(QStringList strListPro,  DependencyModel *selectExternalLibraryList);
QStringList addListOfDependency(DependencyModel *selectExternalLibraryList, QStringList strListPro,
                                int lineNumberWithDependencies);
void writeProFile(QStringList strListPro, QString path);
}

namespace QmldirFileHandler {
QString locateQmldirFile(QString path, QWidget *page);
void writeQmldirFile(QStringList strList, QString path);
QStringList parseQmldirFile(QString qmlPath, QString name, QString fullname);
}

/*!
 *  \brief Write a qmldir-file with added file.
 * \param name The name of the file to be created without the extension,
 * without converting the letters to lower case.
 * \param fullname The name of the file being created with the extension,
 *  with the translation of letters to lower case.
 * \param path Path .spec-file.
 * \param page parent widget for file selection dialog
 */
void Qmldir::addFileInQmldir(QString path, QString name, QString fullname, QWidget *page)
{
    QString qmlPath = QmldirFileHandler::locateQmldirFile(path, page), str;
    if (QFile::exists(qmlPath)) {
        QStringList strList = QmldirFileHandler::parseQmldirFile(qmlPath, name, fullname);
        QmldirFileHandler::writeQmldirFile(strList, qmlPath);
    }
}

/*!
 * \brief locateQmldirFile Find the path to the qmldir-file.
 * Returns the \c QString path to the qmldir-file.
 * \param path Start directory for file search.
 * \param page parent widget for file selection dialog
 */
QString QmldirFileHandler::locateQmldirFile(QString path, QWidget *page)
{
    QString qmlPath;
    QDir pathDir(path);
    QStringList qmldirFiles, filters;
    ChoiceFileDialog *dialog = new ChoiceFileDialog(page);
    int index = 0;
    filters << "*";
    pathDir.setNameFilters(filters);
    pathDir.setFilter(QDir::Files);
    for (QString str : pathDir.entryList()) {
        if (!str.contains("."))
            qmldirFiles << str;

    }
    if (qmldirFiles.size() > 1)
        index = dialog->getIndexFile(&qmldirFiles);

    qmlPath = QDir::toNativeSeparators(path + "/" +  qmldirFiles.value(index));
    return qmlPath;
}

/*!
 * \brief writeQmldirFile Write a qmldir-file with added file.
 * \param strList String list yaml contains a qmldir-file.
 * \param qmlPath path qmldir-file.
 */
void QmldirFileHandler::writeQmldirFile(QStringList strList, QString qmlPath)
{
    QFile qmldirFile(qmlPath);
    qmldirFile.open(QFile::WriteOnly);
    for (QString str : strList)
        qmldirFile.write(str.toUtf8());

    qmldirFile.close();
}
/*!
 * \brief parseQmldirFile Does file parsing, checks file duplication.
 * Returns a QStringList qmldir-file.
 * \param qmlPath Path qmldir-file.
 * \param name The name of the file to be created without the extension,
 * without converting the letters to lower case.
 * \param fullname The name of the file being created with the extension,
 *  with the translation of letters to lower case.
 */
QStringList QmldirFileHandler::parseQmldirFile(QString qmlPath, QString name, QString fullname)
{
    QStringList strList;
    QFile qmldirFile(qmlPath);
    QString str, strCheck;
    qmldirFile.open(QFile::ReadOnly);
    while (!qmldirFile.atEnd()) {
        strList << QTextCodec::codecForName("UTF-8")->toUnicode(qmldirFile.readLine());
    }
    qmldirFile.close();
    for (QString str : strList) {
        if (str.contains(fullname)) {
            qmldirFile.close();
            return strList;
        }
    }
    strCheck = strList.value(strList.size() - 1);
    str = name + " 1.0 " + fullname + "\n";
    if (strCheck.remove("\n").isEmpty()) {
        strList.insert(strList.size() - 1, str);
    } else {
        strList << str;
        strList << "\n";
    }
    return strList;
}

/*!
 *  \brief Adds dependencies for selected external libraries to the .yaml-file.
 *  \param path .pro-file path or start directory for search.
 *  \param selectExternalLibraryList is a list of external libraries selected in the wizard
 *  \param page parent widget for file selection dialog
 */
void SailfishProjectDependencies::addDependencyInProFile(const QString &path,
                                                         DependencyModel *selectExternalLibraryList, QWidget *page)
{
    QString proPath = ProFileHandler::locateProFile(path, page);
    if (QFile::exists(proPath)) {
        QStringList strListPro = ProFileHandler::readProFile(path, selectExternalLibraryList);
        ProFileHandler::writeProFile(strListPro, path);
    }
}

/*!
 *  \brief Adds dependencies for selected external libraries to the .spec-file.
 *  \param path Start directory for file search.
 *  \param selectExternalLibraryList is a list of external libraries selected in the wizard
 *  \param page parent widget for file selection dialog
 */
void SailfishProjectDependencies::addDependecyInYamlFile(const QString &dir,
                                                         DependencyModel *selectExternalLibraryList, QWidget *page)
{
    QString filePathYaml = YamlFileHandler::locateYamlFile(dir, page);
    if (QFile::exists(filePathYaml) && (filePathYaml.contains(".yaml"))) {
        QStringList strListYaml = YamlFileHandler::readYamlFile(filePathYaml, selectExternalLibraryList);
        YamlFileHandler::writeYamlFile(strListYaml, filePathYaml);
    } else {
        addDependecyInSpecFile(dir, selectExternalLibraryList, page);
    }
}

/*!
 *  \brief Adds dependencies for selected external libraries to the .pro-file.
 *  \param path Start directory for file search.
 *  \param selectExternalLibraryList is a list of external libraries selected in the wizard
 *  \param page parent widget for file selection dialog
 */
void SailfishProjectDependencies::addDependecyInSpecFile(const QString &dir,
                                                         DependencyModel *selectExternalLibraryList, QWidget *page)
{
    QString filePathSpec = SpecFileHandler::locateSpecFile(dir, page);
    if (QFile::exists(filePathSpec) && (filePathSpec.contains(".spec"))) {
        QStringList strListSpec = SpecFileHandler::readSpecFile(filePathSpec, selectExternalLibraryList);
        SpecFileHandler::writeSpecFile(strListSpec, filePathSpec);
    }
}

/*!
 *  \brief Find the path to the .yaml-file. Returns the \c QString path to the .yaml-file.
 *  \param dir Start directory for file search.
 *  \param page parent widget for file selection dialog
 */
QString YamlFileHandler::locateYamlFile(QString dir, QWidget *page)
{
    ChoiceFileDialog *dialog = new ChoiceFileDialog(page);
    int index = 0;
    QStringList filters;
    QDir projectDir(dir);
    QStringList yamlFiles;
    filters << "*.yaml";
    projectDir.setNameFilters(filters);
    QDir projectDirRpm(QDir::toNativeSeparators(dir + "/rpm"));
    projectDirRpm.setNameFilters(filters);
    yamlFiles = projectDir.entryList();
    for (QString strTemp : projectDirRpm.entryList())
        yamlFiles << QDir::toNativeSeparators("rpm/" + strTemp);

    if (yamlFiles.size() > 1)
        index = dialog->getIndexFile(&yamlFiles);

    QString filePathYaml = QDir::toNativeSeparators(dir + "/" + yamlFiles.value(index));
    return filePathYaml;
}

/*!
 *  \brief Reads a file. Starts parsing the file if the file was considered. Returns the \с QStringList of the file with dependencies.
 *  \param path is the file location
 */
QStringList YamlFileHandler::readYamlFile(QString path, DependencyModel *selectExternalLibraryList)
{
    QFile yamlfile(path);
    QStringList strListYaml;
    if (yamlfile.open(QFile::ReadOnly)) {
        QString strYaml;
        while (!yamlfile.atEnd()) {
            strListYaml << QTextCodec::codecForName("UTF-8")->toUnicode(yamlfile.readLine());
        }
        yamlfile.close();
        strListYaml = parseYamlFile(strListYaml, selectExternalLibraryList);
    }
    return strListYaml;
}

/*!
 *  \brief Parse the file and finds the desired header for dependencies. Returns a \с QStringList .yaml-file with all dependencies.
 *  \param strListYaml String list yaml contains a yaml file.
 *  \param selectExternalLibraryList Select external library list contains the list contains the list of extension libraries selected in the wizard.
 */
QStringList YamlFileHandler::parseYamlFile(QStringList strListYaml,
                                           DependencyModel *selectExternalLibraryList)
{
    QString strYaml;
    if (strListYaml.isEmpty()) {
        strListYaml << "# This section specifies build dependencies\n";
        strListYaml << "\n";
    }
    for (int i = 0; i < selectExternalLibraryList->size(); i++) {

        int lineNumberWithHeading = 0, lineNumberWithDependencies;
        strYaml = strListYaml.value(lineNumberWithHeading);
        while (!(strYaml.contains(selectExternalLibraryList->getYamlHeader(i)))
                && lineNumberWithHeading != strListYaml.size()) {
            lineNumberWithHeading++;
            strYaml = strListYaml.value(lineNumberWithHeading);
        }
        if (lineNumberWithHeading == strListYaml.size()) {
            strListYaml.insert(lineNumberWithHeading + 1, "\n");
            strListYaml.insert(lineNumberWithHeading + 2, selectExternalLibraryList->getYamlHeader(i) + ":\n");
            lineNumberWithDependencies = lineNumberWithHeading + 3;
            strYaml = strListYaml.value(lineNumberWithDependencies);
        } else {
            lineNumberWithDependencies = lineNumberWithHeading + 1;
            strYaml = strListYaml.value(lineNumberWithDependencies);
        }
        strListYaml = YamlFileHandler::addListOfDependencyYaml(selectExternalLibraryList->getDependencyList(
                                                                   i), strListYaml,
                                                               lineNumberWithDependencies, lineNumberWithHeading);
    }
    return strListYaml;
}

/*!
 *  \brief Add a dependency list to a file. Returns a \с QStringList .yaml-file with dependencies.
 *  \param strListYaml String list yaml contains a yaml file.
 *  \param lineNumberWithDependencies Line numer with dependecies is the line number that contains the beginning of the dependency list in .yaml-file.
 *  \param lineNumberWithHeading Line numer with heading is the line number that contains the header for the dependencies.
 *  \param dependecyList This is a list of dependencies to connect to the file.
 */
QStringList YamlFileHandler::addListOfDependencyYaml(QStringList dependecyList,
                                                     QStringList strListYaml,
                                                     int lineNumberWithDependencies, int lineNumberWithHeading)
{
    int lineIndexWithDependencies = lineNumberWithDependencies;
    QStringList dep;
    QString strYaml = strListYaml.value(lineIndexWithDependencies);
    dep.clear();
    for (QString strDep : dependecyList)
        dep << strDep;

    while ((!(strYaml.remove("\n").isEmpty())) && (!(strYaml.contains("#")))
            && (!(strYaml.contains(":"))) && lineIndexWithDependencies < strListYaml.size() + 1) {
        strYaml = strListYaml.value(lineIndexWithDependencies);
        for (QString strDep : dep) {
            if (strYaml.contains(strDep)) {
                dep.removeOne(strDep);
            }
        }
        lineIndexWithDependencies++;
    }
    if (lineIndexWithDependencies > 0 &&  lineIndexWithDependencies != lineNumberWithHeading + 1) {
        lineIndexWithDependencies--;
    }
    dep.removeDuplicates();
    for (QString strDep : dep) {
        strListYaml.insert(lineIndexWithDependencies, "  - " + strDep + "\n");
        lineIndexWithDependencies++;
    }
    return strListYaml;
}

/*!
 *  \brief Write a file with added dependencies.
 *  \param strListYaml String list yaml contains a yaml file.
 *  \param path Path .yaml-file.
 */
void YamlFileHandler::writeYamlFile(QStringList strListYaml, QString path)
{
    QFile yamlfile(path);
    yamlfile.open(QFile::WriteOnly);
    for (QString strWr : strListYaml)
        yamlfile.write(strWr.toUtf8());

    yamlfile.close();
}

/*!
 *  \brief Find the path to the .spec-file. Returns the \c QString path to the .spec-file.
 *  \param dir Start directory for file search.
 *  \param page parent widget for file selection dialog
 */
QString SpecFileHandler::locateSpecFile(QString dir, QWidget *page)
{
    ChoiceFileDialog *dialog = new ChoiceFileDialog(page);
    int index = 0;
    QDir projectDirRpm(QDir::toNativeSeparators(dir + "/" + "rpm"));
    QDir projectDir(dir);
    QStringList filters;
    QStringList specFiles;
    filters.clear();
    filters << "*.spec";
    projectDir.setNameFilters(filters);
    projectDirRpm.setNameFilters(filters);
    specFiles = projectDir.entryList();
    for (QString strTemp : projectDirRpm.entryList())
        specFiles << QDir::toNativeSeparators("rpm/" + strTemp);

    if (specFiles.size() > 1)
        index = dialog->getIndexFile(&specFiles);

    QString filePathSpec = QDir::toNativeSeparators(dir + "/" + specFiles.value(index));
    return filePathSpec;
}

/*!
 *  \brief Reads a file. Starts parsing the file if the file was considered. Returns the \с QStringList of the file with dependencies.
 *  \param path is the file location
 */
QStringList SpecFileHandler::readSpecFile(QString path, DependencyModel *selectExternalLibraryList)
{
    QFile specfile(path);
    QStringList strListSpec;
    if (specfile.open(QFile::ReadWrite)) {
        while (!specfile.atEnd())
            strListSpec << QTextCodec::codecForName("UTF-8")->toUnicode(specfile.readLine());

        specfile.close();
    }
    strListSpec = parseSpecFile(strListSpec, selectExternalLibraryList);
    return strListSpec;
}

/*!
 *  \brief Add a dependency list to a file. Returns a \с QStringList .spec-file with all dependencies.
 *  \param strListSpec String list spec contains a yaml file.
 *  \param selectExternalLibraryList Select external library list contains the list contains the list of extension libraries selected in the wizard.
 */
QStringList SpecFileHandler::parseSpecFile(QStringList strListSpec,
                                           DependencyModel *selectExternalLibraryList)
{
    QString strSpec;
    for (int i = 0 ; i < selectExternalLibraryList->size(); i++) {
        int lineNumberWithHeading = 0, lineNumberWithDependencies;
        strSpec = strListSpec.value(lineNumberWithHeading);
        while (!(strSpec.contains(selectExternalLibraryList->getSpecHeader(i)))
                && lineNumberWithHeading != strListSpec.size()) {
            lineNumberWithHeading++;
            strSpec = strListSpec.value(lineNumberWithHeading);
        }
        if (lineNumberWithHeading == strListSpec.size()) {
            lineNumberWithDependencies = 0;
            strSpec = strListSpec.value(lineNumberWithDependencies);
        } else {
            lineNumberWithDependencies = lineNumberWithHeading;
        }
        strListSpec = addListOfDependencySpec(selectExternalLibraryList->getDependencyList(i),
                                              selectExternalLibraryList->getSpecHeader(i), selectExternalLibraryList->getYamlHeader(i),
                                              strListSpec, lineNumberWithDependencies,
                                              lineNumberWithHeading);
    }
    return strListSpec;
}

/*!
 *  \brief Parse the file and finds the desired header for dependencies.Returns a \с QStringList .spec-file with dependencies.
 *  \param strListSpec String list spec contains a spec file.
 *  \param lineNumberWithDependencies Line numer with dependecies is the line number that contains the beginning of the dependency list in .spec-file.
 *  \param lineNumberWithHeading Line numer with heading is the line number that contains the header for the dependencies.
 *  \param dependecyList is a list of dependencies to connect to the file.
 *  \param specHeader is a dependency header.
 */
QStringList SpecFileHandler::addListOfDependencySpec(QStringList dependecyList, QString specHeader,
                                                     QString yamlHeader,
                                                     QStringList strListSpec, int lineNumberWithDependencies, int lineNumberWithHeading)
{
    int lineIndexWithDependencies = lineNumberWithDependencies;
    QStringList dep;
    QString strSpec = strListSpec.value(lineIndexWithDependencies);
    dep.clear();
    for (QString strDep : dependecyList) {
        dep << strDep;
    }
    while ((!(strSpec.contains("#"))) && strSpec.contains(specHeader)
            && lineIndexWithDependencies < strListSpec.size() + 1) {
        strSpec = strListSpec.value(lineIndexWithDependencies);
        for (QString strDep : dep) {
            if (strSpec.contains(strDep)) {
                dep.removeOne(strDep);
            }
        }
        lineIndexWithDependencies++;
    }
    if (lineIndexWithDependencies > 0 && lineIndexWithDependencies != lineNumberWithHeading + 1) {
        lineIndexWithDependencies--;
    }
    dep.removeDuplicates();
    for (QString strDep : dep) {
        QString dependencyString;
        if (yamlHeader == "PkgConfigBR")
            dependencyString = specHeader + ":   pkgconfig(" + strDep + ")" + "\n";
        else
            dependencyString = specHeader + ":   " + strDep + "\n";
        strListSpec.insert(lineIndexWithDependencies, dependencyString);
        lineIndexWithDependencies++;
    }
    return strListSpec;
}

/*!
 *  \brief Write a file with added dependencies.
 *  \param strListSpec String list yaml contains a spec file.
 *  \param path Path .spec-file.
 */
void SpecFileHandler::writeSpecFile(QStringList strListSpec, QString path)
{
    QFile specfile(path);
    specfile.open(QFile::WriteOnly);
    for (QString strWr : strListSpec)
        specfile.write(strWr.toUtf8());

    specfile.close();
}

/*!
 *  \brief Find the path to the .pro-file. Returns the \c QString path to the .pro-file.
 *  \param dir Start directory for file search.
 *  \param page parent widget for file selection dialog
 */
QString ProFileHandler::locateProFile(QString path, QWidget *page)
{
    QString proPath = path;
    QStringList filters, proFiles;
    if (!path.contains(".pro")) {
        ChoiceFileDialog *dialog = new ChoiceFileDialog(page);
        int index = 0;
        QDir projectDir(path);
        filters << "*.pro";
        projectDir.setNameFilters(filters);
        proFiles = projectDir.entryList();
        if (proFiles.size() > 1)
            index = dialog->getIndexFile(&proFiles);

        proPath = QDir::toNativeSeparators(path + "/" + proFiles.value(index));
    }
    return proPath;
}

/*!
 *  \brief Reads a file. Starts parsing the file if the file was considered. Returns the \с QStringList of the file with dependencies.
 *  \param path is the file location
 */
QStringList ProFileHandler::readProFile(QString path, DependencyModel *selectExternalLibraryList)
{
    QFile profile(path);
    QStringList strListPro;
    if (profile.open(QFile::ReadOnly)) {
        while (!profile.atEnd())
            strListPro << QString::fromUtf8(profile.readLine());

        profile.close();
        strListPro = parseProFile(strListPro, selectExternalLibraryList);
    }
    return strListPro;
}

/*!
 *  \brief Add a dependency list to a file. Returns a \с QStringList .pro-file with all dependencies.
 *  \param strListPro String list contains a .pro-file.
 *  \param selectExternalLibraryList is a list of external libraries selected in the wizard.
 */
QStringList ProFileHandler::parseProFile(QStringList strListPro,
                                         DependencyModel *selectExternalLibraryList)
{
    QString strPro;
    if (strListPro.isEmpty()) {
        strListPro << "#1 Qt Creator linking\n";
        strListPro << "\n";
    }
    int lineNumberWithHeading = 0, lineNumberWithDependencies;
    strPro = strListPro.value(lineNumberWithHeading);
    while (!(((strPro.remove(" ")).contains("QT+=")) || strPro.contains("Qt Creator linking"))
            && lineNumberWithHeading != strListPro.size()) {
        lineNumberWithHeading++;
        strPro = strListPro.value(lineNumberWithHeading);
    }
    if (strPro.contains("Qt Creator linking") || lineNumberWithHeading == strListPro.size()) {
        strListPro.insert(lineNumberWithHeading + 1, "\n");
        lineNumberWithDependencies = lineNumberWithHeading + 2;
        strPro = strListPro.value(lineNumberWithDependencies);
    } else {
        lineNumberWithDependencies = lineNumberWithHeading;
    }
    strListPro = addListOfDependency(selectExternalLibraryList,  strListPro,
                                     lineNumberWithDependencies);
    return strListPro;
}

/*!
 *  \brief Parse the file and finds the desired header for dependencies. Returns a \с QStringList .pro-file with dependencies.
 *  \param strListPro String list contains a .pro-file.
 *  \param lineNumberWithDependencies Line numer with dependecies is the line number that contains the beginning of the dependency list in .pro-file.
 *  \param selectExternalLibraryList is a list of external libraries selected in the wizard.
 */
QStringList ProFileHandler::addListOfDependency(DependencyModel *selectExternalLibraryList,
                                                QStringList strListPro,
                                                int lineNumberWithDependencies)
{
    int lineIndexWithDependencies = lineNumberWithDependencies;
    QStringList qtList;
    QString strPro = strListPro.value(lineIndexWithDependencies);
    int rowOrderCounter = 0, countDepInProFile = 0;
    for (int i = 0; i < selectExternalLibraryList->size(); i++) {
        for (QString strExternalLibraryes : selectExternalLibraryList->getQtList(i)) {
            qtList << strExternalLibraryes;
        }
    }
    while ((!(strPro.remove("\n").isEmpty())) && (!(strPro.contains("#")))) {
        countDepInProFile++;
        qtList.insert(rowOrderCounter,
                      strPro.remove("+=").remove("QT").remove("\n").remove("\\").remove(" "));
        rowOrderCounter++;
        strListPro.removeAt(lineIndexWithDependencies);
        strPro = strListPro.value(lineIndexWithDependencies);
    }
    qtList.removeDuplicates();
    if (qtList.size() > 1) {
        strListPro.insert(lineIndexWithDependencies, "QT += \\\n");
        lineIndexWithDependencies++;
        if (countDepInProFile > 1) {
            qtList.removeFirst();
        }
        for (QString strExternalLibraryes : qtList) {
            strListPro.insert(lineIndexWithDependencies, "    " + strExternalLibraryes + " \\\n");
            lineIndexWithDependencies++;
        }
    } else {
        for (QString strExternalLibraryes : qtList) {
            strListPro.insert(lineIndexWithDependencies, "QT += " + strExternalLibraryes + "\n");
            lineIndexWithDependencies++;
        }
    }
    return strListPro;
}

/*!
 *  \brief Write a file with added dependencies.
 *  \param strListPro String list contains a .pro-file.
 *  \param path Path .pro-file.
 */
void ProFileHandler::writeProFile(QStringList strListPro, QString path)
{
    QFile profile(path);
    if (profile.open(QFile::WriteOnly)) {
        for (QString strWr : strListPro)
            profile.write(strWr.toUtf8());

        profile.close();
    }
}

} // namespace Internal
} // namespace SailfishWizards

