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

#include "common.h"
#include "desktopdocument.h"
#include "utils/fileutils.h"
#include "sailfishwizardsconstants.h"

namespace SailfishWizards {
namespace Internal {

using Core::IEditor;
using Core::IDocument;
using Core::Id;

/*!
 * \brief Constructor for the document object.
 * \param editor Editor associated with this document.
 * \param editorWidget Widget for interacting with the document and changing it's data
 */
DesktopDocument::DesktopDocument(IEditor *editor, DesktopEditorWidget *editorWidget)
    : IDocument(editor)
    , m_editorWidget(editorWidget)
{
    setId(Id("DesktopDocument"));
    connect(editorWidget, &DesktopEditorWidget::contentModified, this, &DesktopDocument::requestSave);
}

/*!
 * \brief Sets if the document is \a modified.
 * Emits \e changed() signal, when the state of the document changes.
 * \param modified Value for setting to the document's status.
 * \sa isModified()
 */
void DesktopDocument::setModified(bool modified)
{
    if (m_modified == modified)
        return;
    m_modified = modified;
    emit changed();
}

/*!
 * \brief Returns \c true if document was modified, otherwise returns false.
 */
bool DesktopDocument::isModified() const
{
    return m_modified;
}

/*!
 * \brief Saves the modified document data. Returns \c true if data were saved, otherwise returns false.
 * \param errorMessage Output parameter for saving message about possible errors.
 * \param fileName Path to the editable file.
 * \param autoSave Flag, which says whether saving happens automatically or not.
 * \c true if saving happens automatically, otherwise \c false.
 */
bool DesktopDocument::save(QString *errorMessage, const QString &fileName, bool autoSave)
{
    Q_UNUSED(autoSave);
    Q_UNUSED(errorMessage);
    bool iconIsEmpty = false;
    const Utils::FileName fileNameToUse = fileName.isEmpty() ? filePath() : Utils::FileName::fromString(
                                              fileName);
    QFile file(fileNameToUse.toString());
    m_fileName = file.fileName();
    m_newIcons = m_editorWidget->iconPaths();

    const QStringList resolutions = Common::getStandartIconResolutions();

    if (m_oldIcons != m_newIcons) {
        if (!m_newIcons.value("General").isEmpty()
                || (m_editorWidget->isManualChecked()
                    && !m_newIcons.value(resolutions[0]).isEmpty()
                    && !m_newIcons.value(resolutions[1]).isEmpty()
                    && !m_newIcons.value(resolutions[2]).isEmpty()
                    && !m_newIcons.value(resolutions[3]).isEmpty())) {

            if (!m_editorWidget->validateIcons())
                return true;

            countIconIndex();
            addIconFiles();
            writeIconsToPro();
            writeIconsToYaml();
            writeIconsToSpec();

            m_editorWidget->reloadIconPage(m_applicationPath, m_iconsIndex);
            m_oldIcons = m_editorWidget->iconPaths();
        } else {
            if (!m_editorWidget->proceedEmptyIcon())
                return true;
            else
                iconIsEmpty = true;
        }
    }
    if (file.open(QFile::WriteOnly)) {
        if (!iconIsEmpty) m_editorWidget->setIconValue(m_applicationPath, m_iconsIndex);
        QString content = m_editorWidget->content();
        file.write(content.toLocal8Bit());
        setModified(false);
        setFilePath(fileNameToUse);
        return true;
    }
    return false;
}

/*!
 * \brief Opens the file and initializes widget with the file's data.
 * Returns whether the file was opened and read successfully.
 * \param errorString Output parameter for saving message about possible errors.
 * \param fileName Path to the associated file.
 * \param realFileName Name of the auto save that should be loaded if the editor is opened from an auto save file.
 * It's the same as \a fileName if the editor is opened from a regular file.
 * \sa OpenResult
 */
IDocument::OpenResult DesktopDocument::open(QString *errorString, const QString &fileName,
                                            const QString &realFileName)
{
    Q_UNUSED(errorString);
    Q_UNUSED(realFileName);
    QFile file(fileName);
    if (file.open(QFile::ReadOnly)) {
        m_fileName = file.fileName();
        m_applicationPath = QFileInfo(file).dir().path();
        setFilePath(Utils::FileName::fromString(fileName));

        m_editorWidget->setContent(file.readAll(), m_applicationPath, Common::getStandartIconResolutions());
        m_oldIcons = m_editorWidget->iconPaths();
        m_iconsIndex = QFileInfo(
                           m_oldIcons.value(Common::getStandartIconResolutions()[3])).fileName().split("_").last().split(".").first().toInt();
        setModified(false);
        return OpenResult::Success;
    }
    return OpenResult::ReadError;
}

/*!
 * \brief Reopens the file that is already opened.
 * Returns \c true if the file was opened and read successfully, otherwise returns \c false.
 * \param errorString Output parameter for saving message about possible errors.
 */
bool DesktopDocument::reload(QString *errorString, ReloadFlag flag, ChangeType type)
{
    Q_UNUSED(flag);
    Q_UNUSED(type);
    m_editorWidget->clearTranslations();
    if (open(errorString, m_fileName, m_fileName) == OpenResult::Success)
        return true;
    else
        return false;
}

/*!
 * \brief This slot sets the modified status to \c true, when the associated signal is emitted.
 * The slot wraps the proper \c setModified() method, because of its non-parameter prototype, which is much more comfortable to use when connecting signals.
 * \sa setModified()
 */
void DesktopDocument::requestSave()
{
    setModified();
}

/*!
 * \brief Counts the unoccupied index for naming and adding of new set of icons to configuration files and target project directory.
 * \sa addIconFiles(), writeIconsToPro(), writeIconsToYaml(), writeIconsToSpec()
 */
void DesktopDocument::countIconIndex()
{
    QDir projectDir(m_applicationPath);
    QRegExp iconsPrefixCheck(SailfishWizards::Constants::REG_EXP_FOR_ICONS_PROFILE_PREFIX);

    const QString proFilePath = Common::getProFilePath(m_applicationPath);
    if (!proFilePath.isEmpty()) {
        QFile profile(proFilePath);
        if (profile.open(QIODevice::ReadOnly)) {
            m_iconsIndex = 1;
            QStringList strListPro;
            while (!profile.atEnd())
                strListPro << QString::fromUtf8(profile.readLine());
            profile.close();
            for (int k = 0; k < strListPro.size(); k++)
                if (iconsPrefixCheck.exactMatch(strListPro.at(k)))
                    m_iconsIndex = m_iconsIndex + 1;
        }
    }
}

/*!
 * \brief Creates and adds the new set of icons to the opened desktop file project directory.
 * \sa countIconIndex(), writeIconsToPro(), writeIconsToYaml(), writeIconsToSpec()
 */
void DesktopDocument::addIconFiles() const
{
    for (const QString &resolution : Common::getStandartIconResolutions()) {
        if (!QDir(m_applicationPath + resolution).exists())
            QDir(m_applicationPath).mkdir(resolution);

        if (!m_editorWidget->isManualChecked()) {
            int pixelNumber = resolution.split("x").first().toInt();
            QImage(m_newIcons.value("General")).scaled(pixelNumber,
                                                       pixelNumber).save(m_applicationPath + "/" + resolution + "/" + QDir(
                                                                             m_applicationPath).dirName() +
                                                                         "_" + QString::number(m_iconsIndex) + ".png");
        } else {
            QFile::copy(m_newIcons.value(resolution),
                        m_applicationPath + "/" + resolution + "/" + QDir(m_applicationPath).dirName() +
                        "_" + QString::number(m_iconsIndex) + ".png");
        }

    }
}

/*!
 * \brief Writes the information about created icon set to the opened desktop file's main project file.
 * \sa countIconIndex(), addIconFiles(), writeIconsToYaml(), writeIconsToSpec()
 */
void DesktopDocument::writeIconsToPro() const
{
    const QString proFilePath = Common::getProFilePath(m_applicationPath);
    if (!proFilePath.isEmpty()) {
        QFile profile(proFilePath);
        if (profile.open(QIODevice::ReadOnly)) {
            QTextStream writeStream(&profile);
            QStringList strListPro;
            QString profileString;
            while (!profile.atEnd())
                strListPro << QString::fromUtf8(profile.readLine());
            profile.close();
            for (QString resolution : Common::getStandartIconResolutions()) {
                QString pixelNumber = resolution.split("x").first();
                profileString += "\nicon" + pixelNumber + "_" + QString::number(m_iconsIndex) + ".files = " +
                                 resolution + "/" + QDir(
                                     m_applicationPath).dirName() +
                                 "_" + QString::number(m_iconsIndex) + ".png";
                profileString += "\nicon" + pixelNumber + "_" + QString::number(m_iconsIndex) +
                                 ".path = /usr/share/icons/hicolor/" + resolution + "/apps\n";
                profileString += "INSTALLS += icon" + pixelNumber + "_" + QString::number(m_iconsIndex) + "\n";
            }
            int distFilesIndex;
            for (distFilesIndex = 0; distFilesIndex < strListPro.size(); distFilesIndex++)
                if (strListPro.at(distFilesIndex).contains("DISTFILES"))
                    break;

            QString distFilesString = (distFilesIndex == strListPro.size() ? QString("DISTFILES+= ") : "");
            for (const QString &resolution : Common::getStandartIconResolutions())
                distFilesString += QString("    %1/").arg(resolution) + QDir(m_applicationPath).dirName() + "_" + QString::number(m_iconsIndex) + ".png \\\n";

            strListPro.insert(++distFilesIndex, distFilesString);
            strListPro.append(profileString);
            profile.open(QIODevice::WriteOnly | QIODevice::Truncate);
            for (QString strWr : strListPro) {
                profile.write(strWr.toUtf8());
            }
            profile.close();
        }
    }
}

/*!
 * \brief Writes the information about created icon set to the opened desktop file project's spectacle yaml file for installation on the target system.
 * \sa countIconIndex(), addIconFiles(), writeIconsToPro(), writeIconsToSpec()
 */
void DesktopDocument::writeIconsToYaml() const
{
    QStringList filters, yamlFiles;
    filters << "*.yaml";
    QDir projectDirRpm(m_applicationPath + "/rpm");
    projectDirRpm.setNameFilters(filters);
    yamlFiles = projectDirRpm.entryList();

    if (QFile::exists(m_applicationPath + "/rpm/" + yamlFiles.value(0)) && (!yamlFiles.isEmpty())) {
        QFile yamlFile(m_applicationPath + "/rpm/" + yamlFiles.value(0));
        if (yamlFile.open(QIODevice::ReadOnly)) {
            int iconsFileIndex = 1;
            QStringList strListYaml;
            while (!yamlFile.atEnd())
                strListYaml << QTextCodec::codecForName("UTF-8")->toUnicode(yamlFile.readLine());

            yamlFile.close();

            for (int k = 0; k < strListYaml.size(); k++) {
                if (strListYaml.at(k).contains("- '%{_datadir}/icons/hicolor/")) {
                    iconsFileIndex = k;
                    break;
                }
            }

            QString iconString;
            for (QString resolution : Common::getStandartIconResolutions())
                iconString += "  - '%{_datadir}/icons/hicolor/" + resolution + "/apps/" + QDir(
                                  m_applicationPath).dirName() +
                              "_" + QString::number(m_iconsIndex) + ".png'\n";
            strListYaml.insert(iconsFileIndex, iconString);
            yamlFile.open(QIODevice::WriteOnly | QIODevice::Truncate);
            for (const QString &strWr : strListYaml) {
                yamlFile.write(strWr.toUtf8());
            }
            yamlFile.close();
        }
    }
}

/*!
 * \brief Writes the information about created icon set to the opened desktop file project's rpm-build spec file for installation on the target system.
 * \sa countIconIndex(), addIconFiles(), writeIconsToYaml(), writeIconsToPro()
 */
void DesktopDocument::writeIconsToSpec() const
{
    QStringList filters, specFiles;
    filters << "*.spec";
    QDir projectDirRpm(m_applicationPath + "/rpm");
    projectDirRpm.setNameFilters(filters);
    specFiles = projectDirRpm.entryList();

    if (QFile::exists(m_applicationPath + "/rpm/" + specFiles.value(0)) && (!specFiles.isEmpty())) {
        QFile specFile(m_applicationPath + "/rpm/" + specFiles.value(0));
        if (specFile.open(QIODevice::ReadOnly)) {
            int iconsFileIndex = 1;
            QStringList strListSpec;
            while (!specFile.atEnd()) {
                strListSpec << QTextCodec::codecForName("UTF-8")->toUnicode(specFile.readLine());
            }
            specFile.close();
            for (int k = 0; k < strListSpec.size(); k++) {
                if (strListSpec.at(k).contains("%{_datadir}/icons/hicolor/")) {
                    iconsFileIndex = k;
                    break;
                }
            }
            QString iconString;
            for (QString resolution : Common::getStandartIconResolutions()) {
                iconString += "%{_datadir}/icons/hicolor/" + resolution + "/apps/" + QDir(
                                  m_applicationPath).dirName() +
                              "_" + QString::number(m_iconsIndex) + ".png'\n";
            }
            strListSpec.insert(iconsFileIndex, iconString);
            specFile.open(QIODevice::WriteOnly | QIODevice::Truncate);
            for (QString strWr : strListSpec) {
                specFile.write(strWr.toUtf8());
            }
            specFile.close();
        }
    }
}


} // namespace Internal
} // namespace SailfishWizards
