/****************************************************************************
**
** Copyright (C) 2019 Open Mobile Platform LLC
** Contact: https://community.omprussia.ru/open-source
**
** This file is part of Qt Creator.
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

#include <QStandardPaths>
#include "desktopwizardfactory.h"
#include "desktopwizardpages.h"
#include "sailfishwizardsconstants.h"
#include "ui_desktopselectpage.h"
#include "ui_desktopfilesettingpage.h"
#include "ui_desktopiconpage.h"
#include "common.h"

namespace SailfishWizards {
namespace Internal {

/*!
 * \brief Constructs, sets the parameters of the wizard to create a Desktop file.
 */
DesktopWizardFactory::DesktopWizardFactory()
{
    setId("DesktopWizard");
    setIconText("Desktop");
    setDisplayName(tr("Desktop file"));
    setDescription(tr("The wizard allows to create Desktop files for the Sailfish applications."));
    setCategory("Sailfish");
    setDisplayCategory("Sailfish");
    setFlags(IWizardFactory::WizardFlag::PlatformIndependent);
}

/*!
 * \brief Checks platform availability.
 * Returns \c true if the platform is available, otherwise \c false.
 * \param platformId platform identifier.
 */
bool DesktopWizardFactory::isAvailable(Id platformId) const
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
BaseFileWizard *DesktopWizardFactory::create(QWidget *parent,
                                             const WizardDialogParameters &parameters) const
{
    BaseFileWizard *result = new BaseFileWizard(this, parameters.extraValues(), parent);

    result->addPage(new DesktopLocationPage(parameters.defaultPath()));
    result->addPage(new DesktopSettingPage(parameters.defaultPath(),
                                           new TranslationTableModel));
    result->addPage(new DesktopIconPage);

    QListIterator<QWizardPage *> extra_pages(result->extensionPages());
    while (extra_pages.hasNext()) {
        result->addPage(extra_pages.next());
    }

    QRect window = QGuiApplication::primaryScreen()->geometry();
    result->setMinimumSize(static_cast<int>(window.width() * 0.5),
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
GeneratedFiles DesktopWizardFactory::generateFiles(const QWizard *wizard,
                                                   QString *errorMessage) const
{
    GeneratedFiles files;

    errorMessage->clear();
    GeneratedFile desktopFile = generateDesktopFile(wizard, errorMessage);
    if (!errorMessage->isEmpty()) {
        return files;
    }
    files << desktopFile;

    return files;
}

/*!
 * \brief Returns an extension of .desktop file.
 */
QString DesktopWizardFactory::desktopSuffix() const
{
    return preferredSuffix(SailfishWizards::Constants::DESKTOP_MIMETYPE);
}

/*!
 * \brief Sets the file to create. Parses the fields of the
 * wizard and inserts the data into this file.
 * Returns file to create.
 * \param wizard Wizard for which files are created.
 * \param errorMessage The string to display the error message.
 */
GeneratedFile DesktopWizardFactory::generateDesktopFile(const QWizard *wizard,
                                                        QString *errorMessage) const
{
    GeneratedFile desktopFile;
    QHash <QString, QString> values;
    values.insert("appName", wizard->field("appName").toString());
    if (wizard->field("privileges").toString() == "true")
        values.insert("Exec", "invoker --type=silica-qt5 -s " + wizard->field("appPath").toString());
    else
        values.insert("Exec", wizard->field("appPath").toString());

    DesktopLocationPage *locationPage = static_cast<DesktopLocationPage *>(wizard->page(PageId::LOCATION));
    const QString applicationPath = locationPage->getDefaultPath();

    locationPage->findIconsInsertingLine();

    if (!wizard->field("iconPath").toString().isEmpty()
            || !wizard->field("icon172Path").toString().isEmpty())
        values.insert("icons", "\nIcon=" + QDir(applicationPath).dirName() + "_" + QString::number(
                          locationPage->getIconsIndex()));
    else
        values.insert("icons", "");

    QString translationsStr;
    DesktopSettingPage *settingPage = static_cast<DesktopSettingPage *>
                                      (wizard->page(PageId::OPTIONS));
    QList<TranslationData> translations = settingPage->getLangTableModel()->getTranslationData();
    bool isEmpty = true;
    for (const TranslationData &td : translations) {
        if (!td.getTranslation().isEmpty())
            isEmpty = false;
    }

    if (!isEmpty) {
        for (int i = 0; i < translations.size(); i++) {
            if (translations.at(i).getTranslation() != "") {
                TranslationData td = translations.at(i);
                translationsStr += "Name[" + td.getLanguageCodeMap().lowerBound(translations.at(
                                                                                    i).getLanguage()).value() + "]=" +
                                   translations.at(i).getTranslation() + "\n";
            }
        }
        values.insert("translations", "\n\n" + translationsStr.trimmed());
    } else {
        values.insert("translations", "");
    }

    QString fileContents = Common::processTemplate("file.desktop", values, errorMessage);
    if (!errorMessage->isEmpty()) {
        return desktopFile;
    }
    QString filePath = buildFileName(wizard->field("Path").toString(), wizard->field("Name").toString(),
                                     desktopSuffix());
    desktopFile.setPath(filePath);
    desktopFile.setContents(fileContents);
    return desktopFile;
}

/*!
 * \brief Performs actions after creating the file.
 * Returns true if successful, otherwise false.
 * \param w wizard Wizard for which files are created.
 * \param l Contains information about the generated files.
 * \param errorMessage The string to display the error message.
 */
bool DesktopWizardFactory::postGenerateFiles(const QWizard *w, const GeneratedFiles &l,
                                             QString *errorMessage) const
{
    Q_UNUSED(l);
    Q_UNUSED(errorMessage);
    addDesktopToProFile(w);
    addDesktopToYamlFile(w);
    addDesktopToSpecFile(w);
    if (!w->field("iconPath").toString().isEmpty() || (w->field("manualInputCheck").toBool() &&
                                                       !w->field("icon172Path").toString().isEmpty())) {
        addIconToDesktopFile(w);
        addIconsToProject(w);
    }
    return true;
}

/*!
 * \brief Create icons directory and place the icons of the required size in this directory.
 */
void DesktopWizardFactory::addIconToDesktopFile(const QWizard *wizard) const
{
    DesktopLocationPage *locationPage = static_cast<DesktopLocationPage *>
                                        (wizard->page(PageId::LOCATION));
    const QString applicationPath = locationPage->getDefaultPath();
    const QString iconPath = wizard->field("iconPath").toString();

    int iconsIndex = locationPage->getIconsIndex();

    for (const QString &resolution : Common::getStandartIconResolutions()) {
        QDir(applicationPath).mkdir(resolution);
        int pixelNumber = resolution.split("x").first().toInt();

        if (!wizard->field("manualInputCheck").toBool()) {
            QImage(iconPath).scaled(pixelNumber, pixelNumber).save(QDir::toNativeSeparators(applicationPath + QString("/%1/").arg(resolution) +
                                                                       QDir(applicationPath).dirName() +
                                                                       "_" + QString::number(iconsIndex) + ".png"));
        } else {
            QFile::copy(wizard->field(QString("icon%1Path").arg(pixelNumber)).toString(),
                        QDir::toNativeSeparators(applicationPath + QString("/%1/").arg(resolution) +
                                                 QDir(applicationPath).dirName() +
                                                 "_" + QString::number(iconsIndex) + ".png"));
        }

    }
}

/*!
 * \brief Adds code to the .pro file to install the
 * .desktop file and application icons to the
 * project file system.
 */
void DesktopWizardFactory::addDesktopToProFile(const QWizard *wizard) const
{
    DesktopLocationPage *locationPage = static_cast<DesktopLocationPage *>
                                        (wizard->page(PageId::LOCATION));
    const QString applicationPath = locationPage->getDefaultPath();
    int iconsIndex = locationPage->getIconsIndex();
    const QString genericDataLocation = QStandardPaths::standardLocations(
                                            QStandardPaths::GenericDataLocation).last();
    QRegExp desktopPrefixCheck(SailfishWizards::Constants::REG_EXP_FOR_DESKTOP_FILE_PROFILE_PREFIX);
    QRegExp iconsPrefixCheck(SailfishWizards::Constants::REG_EXP_FOR_ICONS_PROFILE_PREFIX);

    const QString proFilePath = Common::getProFilePath(applicationPath);
    if (!proFilePath.isEmpty()) {
        QFile profile(proFilePath);
        if (profile.open(QIODevice::ReadWrite)) {
            QTextStream writeStream(&profile);
            int desktopIndex = 1;
            QStringList strListPro;
            while (!profile.atEnd())
                strListPro << QString::fromUtf8(profile.readLine());
            for (int k = 0; k < strListPro.size(); k++)
                if (desktopPrefixCheck.exactMatch(strListPro.at(k)))
                    desktopIndex++;

            QString profileString = "\ndesktop" + QString::number(desktopIndex) + ".files = " +
                                    wizard->field("Name").toString() +
                                    "."
                                    + desktopSuffix();
            profileString += "\ndesktop" + QString::number(desktopIndex) + ".path = " +
                             QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation).last() + "\n";
            profileString += "INSTALLS += desktop" + QString::number(desktopIndex) + "\n";

            if (!wizard->field("iconPath").toString().isEmpty() ||
                    (wizard->field("manualInputCheck").toBool() && !wizard->field("icon172Path").toString().isEmpty())) {

                for (const QString &resolution : Common::getStandartIconResolutions()) {
                    int pixelNumber = resolution.split("x").first().toInt();

                    profileString += QString("\nicon%1_").arg(pixelNumber) + QString::number(iconsIndex) + QString(".files = %1/").arg(resolution)
                            +  QDir(applicationPath).dirName() + "_" + QString::number(iconsIndex) + ".png";
                    profileString += QString("\nicon%1_").arg(pixelNumber) + QString::number(iconsIndex) +
                                     ".path = " + genericDataLocation + QString("/icons/hicolor/%1/apps\n").arg(resolution);
                    profileString += QString("INSTALLS += icon%1_").arg(pixelNumber) + QString::number(iconsIndex) + "\n";
                }
            }
            writeStream << QDir::toNativeSeparators(profileString);
            profile.close();
        }
    }
}

/*!
 * \brief Adds icons to the project.
 */
void DesktopWizardFactory::addIconsToProject(const QWizard *wizard) const
{
    DesktopLocationPage *locationPage = static_cast<DesktopLocationPage *>
                                        (wizard->page(PageId::LOCATION));
    const QString applicationPath = locationPage->getDefaultPath();
    int iconsIndex = locationPage->getIconsIndex();

    const QString proFilePath = Common::getProFilePath(applicationPath);
    if (!proFilePath.isEmpty()) {
        QFile profile(proFilePath);
        if (profile.open(QIODevice::ReadOnly)) {
            int distFilesIndex = 1;
            QStringList strListPro;
            while (!profile.atEnd())
                strListPro << QString::fromUtf8(profile.readLine());
            profile.close();

            for (int k = 0; k < strListPro.size(); k++)
                if (strListPro.at(k).contains("DISTFILES")) {
                    distFilesIndex = k + 1;
                    break;
                }
            QString distFilesString;
            for (const QString &resolution : Common::getStandartIconResolutions())
                distFilesString += QString("    %1/").arg(resolution) + QDir(applicationPath).dirName() + "_" + QString::number(iconsIndex) + ".png \\\n";

            strListPro.insert(distFilesIndex, QDir::toNativeSeparators(distFilesString));
            profile.open(QIODevice::WriteOnly | QIODevice::Truncate);
            for (QString strWr : strListPro) {
                profile.write(strWr.toUtf8());
            }
            profile.close();
        }
    }
}

/*!
 * \brief Adds code to the yaml file to install the
 * .desktop file and application icons to SailfishOS.
 */
void DesktopWizardFactory::addDesktopToYamlFile(const QWizard *wizard) const
{
    int index = 0;
    DesktopLocationPage *locationPage = static_cast<DesktopLocationPage *>
                                        (wizard->page(PageId::LOCATION));
    QString applicationPath = locationPage->getDefaultPath();
    int iconsIndex = locationPage->getIconsIndex();
    QDir projectDir(applicationPath);
    QStringList filters, yamlFiles;
    filters << "*.yaml";
    QDir projectDirRpm(QDir::toNativeSeparators(applicationPath + "/rpm"));
    projectDirRpm.setNameFilters(filters);
    yamlFiles = projectDirRpm.entryList();

    if (QFile::exists(QDir::toNativeSeparators(applicationPath + "/rpm/" + yamlFiles.value(index)))
            && (!yamlFiles.isEmpty())) {
        QFile yamlFile(QDir::toNativeSeparators(applicationPath + "/rpm/" + yamlFiles.value(index)));
        if (yamlFile.open(QIODevice::ReadOnly)) {
            int desktopFileIndex = 1;
            int iconsFileIndex = 1;
            QStringList strListYaml;
            while (!yamlFile.atEnd())
                strListYaml << QTextCodec::codecForName("UTF-8")->toUnicode(yamlFile.readLine());
            yamlFile.close();
            for (int k = 0; k < strListYaml.size(); k++)
                if (strListYaml.at(k).contains("Files:")) {
                    bool desktopIndexSet = false;
                    bool iconsIndexSet = false;
                    for (; k < strListYaml.size(); k++) {
                        if (strListYaml.at(k).contains(QDir::toNativeSeparators("- '%{_datadir}/applications/"))
                                && !desktopIndexSet) {
                            desktopFileIndex = k;
                            desktopIndexSet = true;
                            if (iconsIndexSet)
                                break;
                        }
                        if (strListYaml.at(k).contains(QDir::toNativeSeparators("- '%{_datadir}/icons/hicolor/"))
                                && !iconsIndexSet) {
                            iconsFileIndex = k;
                            iconsIndexSet = true;
                            if (desktopIndexSet)
                                break;
                        }
                    }
                }
            QString yamlFileString = "  - '%{_datadir}/applications/" + wizard->field("Name").toString() + "." +
                                     desktopSuffix() + "'\n";
            strListYaml.insert(desktopFileIndex, QDir::toNativeSeparators(yamlFileString));

            QString iconString;
            for (const QString &resolution : Common::getStandartIconResolutions())
                iconString += QString("  - '%{_datadir}/icons/hicolor/%1/apps/").arg(resolution)
                        + QDir(applicationPath).dirName() + "_" + QString::number(iconsIndex) + ".png'\n";

            if (!wizard->field("iconPath").toString().isEmpty() || (wizard->field("manualInputCheck").toBool()
                                                                    &&
                                                                    !wizard->field("icon172Path").toString().isEmpty()))
                strListYaml.insert(iconsFileIndex + 1, QDir::toNativeSeparators(iconString));

            yamlFile.open(QIODevice::WriteOnly | QIODevice::Truncate);
            for (QString strWr : strListYaml) {
                yamlFile.write(strWr.toUtf8());
            }
            yamlFile.close();
        }
    }
}

/*!
 * \brief Adds code to the spec file to install the
 * .desktop file and application icons to SailfishOS.
 */
void DesktopWizardFactory::addDesktopToSpecFile(const QWizard *wizard) const
{
    int index = 0;
    DesktopLocationPage *locationPage = static_cast<DesktopLocationPage *>
                                        (wizard->page(PageId::LOCATION));
    QString applicationPath = locationPage->getDefaultPath();
    int iconsIndex = locationPage->getIconsIndex();
    QDir projectDir(applicationPath);
    QStringList filters, specFiles;
    filters << "*.spec";
    QDir projectDirRpm(QDir::toNativeSeparators(applicationPath + "/rpm"));
    projectDirRpm.setNameFilters(filters);
    specFiles = projectDirRpm.entryList();

    if (QFile::exists(QDir::toNativeSeparators(applicationPath + "/rpm/" + specFiles.value(index)))
            && (!specFiles.isEmpty())) {
        QFile specFile(QDir::toNativeSeparators(applicationPath + "/rpm/" + specFiles.value(index)));
        if (specFile.open(QIODevice::ReadOnly)) {
            int desktopFileIndex = 1;
            int iconsFileIndex = 1;
            QStringList strListSpec;
            while (!specFile.atEnd())
                strListSpec << QTextCodec::codecForName("UTF-8")->toUnicode(specFile.readLine());
            specFile.close();
            for (int k = 0; k < strListSpec.size(); k++)
                if (strListSpec.at(k).contains("%files")) {
                    bool desktopIndexSet = false;
                    bool iconsIndexSet = false;
                    for (; k < strListSpec.size(); k++) {
                        if (strListSpec.at(k).contains(QDir::toNativeSeparators("%{_datadir}/applications/"))
                                && !desktopIndexSet) {
                            desktopFileIndex = k;
                            desktopIndexSet = true;
                            if (iconsIndexSet)
                                break;
                        }
                        if (strListSpec.at(k).contains(QDir::toNativeSeparators("%{_datadir}/icons/hicolor/"))
                                && !iconsIndexSet) {
                            iconsFileIndex = k;
                            iconsIndexSet = true;
                            if (desktopIndexSet)
                                break;
                        }
                    }
                }
            QString specFileString = "%{_datadir}/applications/" + wizard->field("Name").toString() + "." +
                                     desktopSuffix() + "\n";
            strListSpec.insert(desktopFileIndex, QDir::toNativeSeparators(specFileString));

            QString iconString;
            for (const QString &resolution : Common::getStandartIconResolutions())
                iconString += QString("%{_datadir}/icons/hicolor/%1/apps/").arg(resolution) + QDir(applicationPath).dirName() +
                        "_" + QString::number(iconsIndex) + ".png\n";

            if (!wizard->field("iconPath").toString().isEmpty() || (wizard->field("manualInputCheck").toBool()
                                                                    &&
                                                                    !wizard->field("icon172Path").toString().isEmpty()))
                strListSpec.insert(iconsFileIndex + 1, iconString);

            specFile.open(QIODevice::WriteOnly | QIODevice::Truncate);
            for (QString strWr : strListSpec) {
                specFile.write(strWr.toUtf8());
            }
            specFile.close();
        }
    }
}
}
}
