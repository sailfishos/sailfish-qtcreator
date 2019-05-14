/****************************************************************************
**
** Copyright (C) 2019 Open Mobile Platform LLC
** Contact: https://community.omprussia.ru/open-source
**
** Qt Creator class for creating a wizard for creating QML files.
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

#include "qmlwizardfactory.h"
#include "common.h"
#include "projectexplorer/projectwizardpage.h"
#include "sailfishwizardsconstants.h"
#include "sailfishprojectdependencies.h"
#include "qmlwizardpage.h"
#include "wizardpage.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QStringList>
#include <QScreen>
#include <QTextCodec>
#include <qtsupport/qtsupportconstants.h>

namespace SailfishWizards {
namespace Internal {

using namespace Core;

/*!
 * \brief Constructs, sets the parameters of the wizard to create a QML file.
 */
QmlWizardFactory::QmlWizardFactory()
{
    setId("QmlWizard");
    setIconText("qml");
    setDisplayName(tr("QML"));
    setDescription(tr("The wizard allows to create Qml files for the Sailfish projects."));
    setCategory("Sailfish");
    setDisplayCategory("Sailfish");
    setFlags(IWizardFactory::WizardFlag::PlatformIndependent);
}

/*!
 * \brief Loads external libraries data from the resource json file.
 * Returns model containing loaded libraries data.
 * \sa DependencyModel
 */
DependencyModel *QmlWizardFactory::loadAllExternalLibraries() const
{
    DependencyModel *model = new DependencyModel;
    QRegExp rx(" ");
    QString value;
    ExternalLibrary newExternalLibrary;
    QStringList strList;
    QJsonArray jsonArray;
    QFile jsonFile(":/saifishwizards/qmlwizard/data/qml-modules.json");
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

        jsonArray = val["types"].toArray();
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
bool QmlWizardFactory::isAvailable(Id platformId) const
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
BaseFileWizard *QmlWizardFactory::create(QWidget *parent,
                                         const WizardDialogParameters &parameters) const
{
    QStringList *localList = new QStringList;
    DependencyModel *allExternalLibraryList = loadAllExternalLibraries();
    DependencyModel *selectExternalLibraryList = new DependencyModel;
    BaseFileWizard *result = new BaseFileWizard(this, parameters.extraValues(), parent);
    result->addPage(new LocationPageQml(parameters.defaultPath()));
    result->addPage(new LocalImportsPage(localList));
    result->addPage(new ExternalLibraryPage(allExternalLibraryList, selectExternalLibraryList));
    result->addPage(new OptionPageQml(localList, selectExternalLibraryList));
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
 */
GeneratedFiles QmlWizardFactory::generateFiles(const QWizard *wizard, QString *errorMessage) const
{
    Q_UNUSED(errorMessage);
    GeneratedFile jsLibraryFile;
    GeneratedFiles files;
    QHash <QString, QString> values;
    QString path = wizard->field("path").toString();
    QString name = wizard->field("name").toString();
    if (name.contains(".js"))
        name = name.remove(name.length() - 3, 3);
    QDir filedir(path);
    QString versionQtQuick = wizard->field("versionQtQuick").toString();
    QString baseType = wizard->field("baseType").toString();
    QString strLocalImports, strExternalLibrary;
    OptionPageQml *optionsPage = static_cast<OptionPageQml *>(wizard->page(PageId::OPTIONS));
    DependencyModel *selectExternalLibraryList = optionsPage->getExternalLibraryList();
    QStringList *localList = optionsPage->getLocalList();
    for (int i = 0; i < localList->size(); i++)
        strLocalImports += "import \"" + filedir.relativeFilePath(localList->value(i)) + "\"\n";

    for (int i = 0; i < selectExternalLibraryList->size(); i++) {
        strExternalLibrary += "import " + selectExternalLibraryList->getExternalLibraryByIndex(
                                  i).getName() + "\n";
    }
    values.insert("strExternalLibrary", strExternalLibrary);
    values.insert("baseType", baseType);
    values.insert("localFiles", strLocalImports);
    values.insert("versionQtQuick", versionQtQuick);
    QString fileContents = Common::processTemplate("qmlfile.template", values, errorMessage);

    QString filePath = buildFileName(path, name, qmlSuffix());
    jsLibraryFile.setPath(filePath);
    jsLibraryFile.setContents(fileContents);
    files << jsLibraryFile;
    return files;
}

/*!
 * \brief QmlWizardFactory::qmlSuffix Returns the mime type QML.
 */
QString QmlWizardFactory::qmlSuffix() const
{
    return preferredSuffix(SailfishWizards::Constants::QML_MIMETYPE);
}

/*!
 * \brief Performs actions after creating the file.
 * Returns true if successful, otherwise false.
 * \param w wizard Wizard for which files are created.
 * \param l Contains information about the generated files.
 * \param errorMessage The string to display the error message.
 */
bool QmlWizardFactory::postGenerateFiles(const QWizard *w, const GeneratedFiles &l,
                                         QString *errorMessage) const
{
    Q_UNUSED(l);
    Q_UNUSED(errorMessage);
    QString filePath = w->field("path").toString();
    QString name = w->field("name").toString(), fullname;
    if (name.contains(".js")) {
        fullname = name.toLower();
        name = name.remove(name.length() - 3, 3);
    } else {
        fullname = name.toLower() + ".qml";
    }
    QWizardPage *page =  w->currentPage();
    DependencyModel *selectExternalLibraryList = static_cast<OptionPageQml *>(w->page(
                                                                                  PageId::OPTIONS))->getExternalLibraryList();
    if (selectExternalLibraryList->size() != 0) {
        QFileInfo proFileInfo;
        QString path;

        const LocationPageQml *locationPage = static_cast<const LocationPageQml *>(w->page(PageId::LOCATION));
        if (locationPage) {
            const QString proFilePath = Common::getProFilePath(locationPage->getDefaultPath());
            proFileInfo.setFile(proFilePath);
        }

        if (proFileInfo.exists()) {
            filePath = proFileInfo.path();
            path = proFileInfo.filePath();
        } else {
            path = filePath;
        }
        SailfishProjectDependencies::addDependecyInYamlFile(filePath, selectExternalLibraryList, page);
        SailfishProjectDependencies::addDependencyInProFile(path, selectExternalLibraryList, page);
    }
    Qmldir::addFileInQmldir(filePath, name, fullname, page);
    return true;
}

} // namespace Internal
} // namespace SailfishWizards
