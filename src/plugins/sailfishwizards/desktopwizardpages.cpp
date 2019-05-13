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
#include "desktopwizardpages.h"
#include "desktopwizardfactory.h"
#include "ui_desktopselectpage.h"
#include "ui_desktopfilesettingpage.h"
#include "ui_desktopiconpage.h"
#include "sailfishwizardsconstants.h"
#include <QDialog>
#include <QFileDialog>
#include <QMimeDatabase>

const QStringList m_languagesList = {"Danish", "German", "English (UK)", "English (US)", "Spanish",
                               "French", "Italian", "Norwegian", "Polish", "Portuguese", "Finnish",
                               "Swedish", "Russian", "Marathi", "Hindi", "Bengali", "Punjabi",
                               "Gujarati", "Tamil", "Telugu", "Kannada", "Malayalam",
                               "Chinese (Mainland)", "Chinese (Hong Kong)"
                              };
const QStringList m_languageCodesList = {"da", "de", "en_GB", "en_US", "es_ES", "fr", "it", "no", "pl",
                                   "pt", "fi", "sv_SE", "ru", "mr_IN", "hi_IN", "bn_BD", "pa_IN",
                                   "gu_IN", "ta_IN", "te_IN", "kn_IN", "ml_IN", "zh_CN", "zh_HK"
                                  };

namespace SailfishWizards {
namespace Internal {

/*!
 * \brief The constructor of the location page
 * sets the user interface and the initial
 * location of the file.
 * \param defaultPath Path to the project.
 */
DesktopLocationPage::DesktopLocationPage(const QString &defaultPath)
{
    m_pageUi.setupUi(this);
    if (!QFile(QDir::toNativeSeparators(defaultPath + "/" + QDir(defaultPath).dirName() +
                                        ".desktop")).exists())
        m_pageUi.fileNameEdit->setText(QDir(defaultPath).dirName());

    m_pageUi.pathChooser->setText(defaultPath);
    m_defaultPath = defaultPath;

    QObject::connect(m_pageUi.pathChooserBtn, &QPushButton::clicked,
                     this, &DesktopLocationPage::chooseFileDirectory);

    registerField("Name*", m_pageUi.fileNameEdit);
    registerField("Path*", m_pageUi.pathChooser);
}

/*!
  * \brief Find line number in the .pro
  * file for adding icons.
  */
void DesktopLocationPage::findIconsInsertingLine()
{
    const QString proFilePath = Common::getProFilePath(m_defaultPath);
    if (!proFilePath.isEmpty()) {
        QFile profile(proFilePath);
        if (profile.open(QIODevice::ReadOnly)) {
            m_iconsIndex = 1;
            QStringList strListPro;
            while (!profile.atEnd())
                strListPro << QString::fromUtf8(profile.readLine());
            profile.close();

            QRegExp iconsPrefixCheck(SailfishWizards::Constants::REG_EXP_FOR_ICONS_PROFILE_PREFIX);

            for (int k = 0; k < strListPro.size(); k++)
                if (iconsPrefixCheck.exactMatch(strListPro.at(k)))
                    m_iconsIndex = m_iconsIndex + 1;
        }
    }
}

/*!
 * \brief This is a override function from the \c QWizardPage, responsible for moving to the next page.
 * Returns \c true if the input fields are validated; otherwise  \c false.
 */
bool DesktopLocationPage::isComplete() const
{
    QRegExp nameCheck(SailfishWizards::Constants::REG_EXP_FOR_NAME_FILE);
    return validateInput(!field("Name").toString().isEmpty(), m_pageUi.fileNameEdit,
                         m_pageUi.errorLabel, tr("The file name line should not be empty"))
           && validateInput(nameCheck.exactMatch(field("Name").toString()), m_pageUi.fileNameEdit,
                            m_pageUi.errorLabel, tr("The file name should consist of alternating letters,\n"
                                                    "numbers, points, dashes and underscores"))
           && validateInput(!QFile(BaseFileWizardFactory::buildFileName(
                                       field("Path").toString(), field("Name").toString(), ".desktop")).exists(), m_pageUi.fileNameEdit,
                            m_pageUi.errorLabel, tr("File with this name is already exists"))
           && validateInput(!field("Path").toString().isEmpty(), m_pageUi.pathChooser,
                            m_pageUi.errorLabel, tr("The path line should not be empty"))
           && validateInput(QDir(field("Path").toString()).exists(), m_pageUi.pathChooser,
                            m_pageUi.errorLabel, tr("Directory should be exist"));
}

/*!
 * \brief Highlights the field with incorrect input and displays an error message.
 * \param condition Field's correct status.
 * \param widget Verifiable widget.
 * \param errorLable Widget for input hint.
 * \param errorMessage Hint for input.
 */
bool DesktopLocationPage::validateInput(bool condition, QWidget *widget, QLabel *errorLabel,
                                        const QString &errorMessage)
{
    QPalette palette;
    if (condition) {
        palette.setColor(QPalette::Text, Qt::black);
        widget->setPalette(palette);
        errorLabel->clear();
        return true;
    } else {
        palette.setColor(QPalette::Text, Qt::red);
        widget->setPalette(palette);
        errorLabel->setText(errorMessage);
        return false;
    }
}

/*!
 * \brief Opens a dialog for changing the file location.
 */
void DesktopLocationPage::chooseFileDirectory()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
                                                    QDir::home().path(),
                                                    QFileDialog::ShowDirsOnly
                                                    | QFileDialog::DontResolveSymlinks);
    if (!dir.isEmpty())
        m_pageUi.pathChooser->setText(dir);
}

/*!
 * \brief The constructor of the icon page
 * sets the user interface and manages
 * the selection of icons.
 */
DesktopIconPage::DesktopIconPage()
{
    m_pageUi.setupUi(this);

    QObject::connect(m_pageUi.iconBrowse, &QPushButton::clicked, this,
                     &DesktopIconPage::chooseFileDirectory);
    QObject::connect(m_pageUi.icon172Browse, &QPushButton::clicked, this,
                     &DesktopIconPage::chooseFileDirectory);
    QObject::connect(m_pageUi.icon128Browse, &QPushButton::clicked, this,
                     &DesktopIconPage::chooseFileDirectory);
    QObject::connect(m_pageUi.icon108Browse, &QPushButton::clicked, this,
                     &DesktopIconPage::chooseFileDirectory);
    QObject::connect(m_pageUi.icon86Browse, &QPushButton::clicked, this,
                     &DesktopIconPage::chooseFileDirectory);

    QObject::connect(m_pageUi.manualInputCheck, &QCheckBox::stateChanged, this,
                     &DesktopIconPage::updateInputFields);
    QObject::connect(m_pageUi.iconPath, &QLineEdit::textChanged, this,
                     &DesktopIconPage::updateIconPreview);

    registerField("icon172Path*", m_pageUi.icon172Path);
    registerField("icon128Path*", m_pageUi.icon128Path);
    registerField("icon108Path*", m_pageUi.icon108Path);
    registerField("icon86Path*", m_pageUi.icon86Path);
    registerField("iconPath*", m_pageUi.iconPath);
    registerField("manualInputCheck", m_pageUi.manualInputCheck);
}

/*!
 * \brief Сhanges the preview of the icon when
 * the field of the icon path is edited.
 * \param iconPath User-entered icon path.
 */
void DesktopIconPage::updateIconPreview(QString iconPath)
{
    if (QFile(iconPath).exists()) {
        QPixmap icon = QPixmap(iconPath);
        m_pageUi.sizeLabel->setText("Icon Size:\n" + QString::number(icon.height()) + "x" + QString::number(
                                        icon.width()));
        m_pageUi.iconLabel->setPixmap(icon.scaled(50, 50));
    } else {
        m_pageUi.sizeLabel->clear();
        m_pageUi.iconLabel->clear();
    }
}

/*!
 * \brief This is a override function from the \c QWizardPage, responsible for moving to the next page.
 * Returns \c true if the input fields are validated; otherwise  \c false.
 */
bool DesktopIconPage::isComplete() const
{
    QMimeDatabase db;
    if (field("iconPath").toString().isEmpty() && !m_pageUi.manualInputCheck->isChecked()) {
        m_pageUi.errorLabel->clear();
        return true;
    } else if (field("icon172Path").toString().isEmpty() && field("icon128Path").toString().isEmpty()
               && field("icon108Path").toString().isEmpty()
               && field("icon86Path").toString().isEmpty() && m_pageUi.manualInputCheck->isChecked()) {
        m_pageUi.errorLabel->clear();
        return true;
    } else if (!m_pageUi.manualInputCheck->isChecked())
        return DesktopLocationPage::validateInput(QFile(field("iconPath").toString()).exists(),
                                                   m_pageUi.iconPath,
                                                   m_pageUi.errorLabel, tr("File with this path isn't exists"))
                && DesktopLocationPage::validateInput(db.mimeTypeForFile(
                                                          field("iconPath").toString()).name().contains(
                                                          QDir::toNativeSeparators("image/")),
                                                      m_pageUi.iconPath,
                                                      m_pageUi.errorLabel, tr("File isn't image"));
    else
        return DesktopLocationPage::validateInput(!field("icon172Path").toString().isEmpty(),
                                                   m_pageUi.icon172Path,
                                                   m_pageUi.errorLabel, tr("The path of 172x172 image file should not be empty"))
                && DesktopLocationPage::validateInput(QFile(field("icon172Path").toString()).exists(),
                                                      m_pageUi.icon172Path,
                                                      m_pageUi.errorLabel, tr("File of 172x172 icon with this path isn't exists"))
                && DesktopLocationPage::validateInput(db.mimeTypeForFile(
                                                          field("icon172Path").toString()).name().contains(
                                                          QDir::toNativeSeparators("image/")),
                                                      m_pageUi.icon172Path,
                                                      m_pageUi.errorLabel, tr("File of 172x172 icon with this path isn't image"))
                && DesktopLocationPage::validateInput(QPixmap(field("icon172Path").toString()).height() == 172
                                                      && QPixmap(field("icon172Path").toString()).width() == 172,
                                                      m_pageUi.icon172Path, m_pageUi.errorLabel, tr("This file isn't 172x172 size image"))
                && DesktopLocationPage::validateInput(!field("icon128Path").toString().isEmpty(),
                                                      m_pageUi.icon128Path,
                                                      m_pageUi.errorLabel, tr("The path of 128x128 image file should not be empty"))
                && DesktopLocationPage::validateInput(QFile(field("icon128Path").toString()).exists(),
                                                      m_pageUi.icon128Path,
                                                      m_pageUi.errorLabel, tr("File of 128x128 icon with this path isn't exists"))
                && DesktopLocationPage::validateInput(db.mimeTypeForFile(
                                                          field("icon128Path").toString()).name().contains(
                                                          QDir::toNativeSeparators("image/")),
                                                      m_pageUi.icon128Path,
                                                      m_pageUi.errorLabel, tr("File of 128x128 icon with this path isn't image"))
                && DesktopLocationPage::validateInput(QPixmap(field("icon128Path").toString()).height() == 128
                                                      && QPixmap(field("icon128Path").toString()).width() == 128,
                                                      m_pageUi.icon128Path, m_pageUi.errorLabel, tr("This file isn't 128x128 size image"))
                && DesktopLocationPage::validateInput(!field("icon108Path").toString().isEmpty(),
                                                      m_pageUi.icon108Path,
                                                      m_pageUi.errorLabel, tr("The path of 108x108 image file should not be empty"))
                && DesktopLocationPage::validateInput(QFile(field("icon108Path").toString()).exists(),
                                                      m_pageUi.icon108Path,
                                                      m_pageUi.errorLabel, tr("File of 108x108 icon with this path isn't exists"))
                && DesktopLocationPage::validateInput(db.mimeTypeForFile(
                                                          field("icon108Path").toString()).name().contains(
                                                          QDir::toNativeSeparators("image/")),
                                                      m_pageUi.icon108Path,
                                                      m_pageUi.errorLabel, tr("File of 108x108 icon with this path isn't image"))
                && DesktopLocationPage::validateInput(QPixmap(field("icon108Path").toString()).height() == 108
                                                      && QPixmap(field("icon108Path").toString()).width() == 108,
                                                      m_pageUi.icon108Path, m_pageUi.errorLabel, tr("This file isn't 108x108 size image"))
                && DesktopLocationPage::validateInput(!field("icon86Path").toString().isEmpty(),
                                                      m_pageUi.icon86Path,
                                                      m_pageUi.errorLabel, tr("The path of 86x86 image file should not be empty"))
                && DesktopLocationPage::validateInput(QFile(field("icon86Path").toString()).exists(),
                                                      m_pageUi.icon86Path,
                                                      m_pageUi.errorLabel, tr("File of 86x86 icon with this path isn't exists"))
                && DesktopLocationPage::validateInput(db.mimeTypeForFile(
                                                          field("icon86Path").toString()).name().contains(
                                                          QDir::toNativeSeparators("image/")),
                                                      m_pageUi.icon86Path,
                                                      m_pageUi.errorLabel, tr("File of 86x86 icon with this path isn't image"))
                && DesktopLocationPage::validateInput(QPixmap(field("icon86Path").toString()).height() == 86
                                                      && QPixmap(field("icon86Path").toString()).width() == 86,
                                                      m_pageUi.icon86Path, m_pageUi.errorLabel, tr("This file isn't 86x86 size image"));
}

/*!
 * \brief Clean up page's input fields.
 */
void DesktopIconPage::cleanupPage()
{
    m_pageUi.iconPath->clear();
    m_pageUi.icon172Path->clear();
    m_pageUi.icon128Path->clear();
    m_pageUi.icon108Path->clear();
    m_pageUi.icon86Path->clear();
    m_pageUi.sizeLabel->clear();
    m_pageUi.iconLabel->clear();
    m_pageUi.manualInputCheck->setChecked(false);
}

/*!
 * \brief Сontrols the activation of icons input fields.
 */
void DesktopIconPage::updateInputFields()
{
    const bool manual = m_pageUi.manualInputCheck->isChecked();
    const bool automatic = !manual;

    m_pageUi.icon172Path->setEnabled(manual);
    m_pageUi.icon172Browse->setEnabled(manual);

    m_pageUi.icon128Path->setEnabled(manual);
    m_pageUi.icon128Browse->setEnabled(manual);

    m_pageUi.icon108Path->setEnabled(manual);
    m_pageUi.icon108Browse->setEnabled(manual);

    m_pageUi.icon86Path->setEnabled(manual);
    m_pageUi.icon86Browse->setEnabled(manual);

    m_pageUi.iconPath->setEnabled(automatic);
    m_pageUi.iconBrowse->setEnabled(automatic);

    if (manual) {
        m_pageUi.iconPath->clear();
        m_pageUi.iconLabel->clear();
        m_pageUi.sizeLabel->clear();
    } else {
        m_pageUi.icon172Path->clear();
        m_pageUi.icon128Path->clear();
        m_pageUi.icon108Path->clear();
        m_pageUi.icon86Path->clear();
    }
}

/*!
 * \brief Opens a dialog for changing the icons location.
 */
void DesktopIconPage::chooseFileDirectory()
{
    m_initiator = static_cast<QPushButton *>(sender());
    QString file = QFileDialog::getOpenFileName(this, tr("Open File"),
                                                QDir::home().path(),
                                                tr("Image Files (*.png *.jpg *.bmp)"));
    QPixmap icon = QPixmap(file);
    if (icon.height() != 0) {
        if (m_initiator == m_pageUi.iconBrowse) {
            m_pageUi.iconPath->setText(file);
            m_pageUi.sizeLabel->setText("Icon Size:\n" + QString::number(icon.height()) + "x" + QString::number(
                                            icon.width()));
            m_pageUi.iconLabel->setPixmap(icon.scaled(50, 50));
        }

        if (m_initiator == m_pageUi.icon172Browse)
            m_pageUi.icon172Path->setText(file);
        if (m_initiator == m_pageUi.icon128Browse)
            m_pageUi.icon128Path->setText(file);
        if (m_initiator == m_pageUi.icon108Browse)
            m_pageUi.icon108Path->setText(file);
        if (m_initiator == m_pageUi.icon86Browse)
            m_pageUi.icon86Path->setText(file);
    }
}

/*!
 * \brief The constructor of the translations
 * table model sets the roles of table elements.
 * \param parent The parent object instance.
 */
TranslationTableModel::TranslationTableModel(QObject *parent)
    :  QAbstractTableModel(parent)
{
}

/*!
 * \brief Return the number of table rows.
 * \param parent The parent object instance.
 */
int TranslationTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 24;
}

/*!
 * \brief Return the number of table columns.
 * \param parent The parent object instance.
 */
int TranslationTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 2;
}

/*!
 * \brief Sets translation of application name.
 * \param index Item index.
 * \param value Item index.
 * \param role Role number.
 */
bool TranslationTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || role != Qt::EditRole || m_translations.count() <= index.row()) {
        return false;
    }
    m_translations[index.row()][Column(index.column())] = value;
    emit dataChanged(index, index);
    return true;
}

/*!
 * \brief Sets header for translations table.
 * \param section Header section index.
 * \param orientation Orientation.
 * \param role Role number.
 */
QVariant TranslationTableModel::headerData(int section, Qt::Orientation orientation,
                                            int role) const
{
    if (role != Qt::DisplayRole) {
        return QVariant();
    }

    if (orientation == Qt::Vertical) {
        return QVariant();
    }

    switch (section) {
    case 0:
        return tr("Language");
    case 1:
        return tr("Translation");
    }
    return QVariant();
}

/*!
 * \brief Returns translation of application name.
 * \param index Item index.
 * \param role Role number.
 */
QVariant TranslationTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() ||
            m_translations.count() <= index.row() ||
            (role != Qt::DisplayRole && role != Qt::EditRole)) {
        return QVariant();
    }
    return m_translations[index.row()][ Column(index.column())];
}

/*!
 * \brief Return the flags of model.
 * \param index Item index.
 */
Qt::ItemFlags TranslationTableModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = QAbstractTableModel::flags(index);
    if (index.isValid()) {
        if (index.column() == 1) {
            flags |= Qt::ItemIsEditable;
        }
    }
    return flags;
}

/*!
 * \brief Adds language to translate to translations table.
 * \param languages List of language to translate.
 */
void TranslationTableModel::setLanguageList(const QStringList &languages)
{
    const int row = m_translations.count();
    beginInsertRows(QModelIndex(), row, row + languages.count());
    for (const QString &language : languages) {
        TranslationLocation translation;
        translation[LANGUAGE] = language;
        translation[TRANSLATION] = "";

        m_translations.append(translation);
    }
    endInsertRows();
}

/*!
 * \brief Clears all translations fields of
 * translations table.
 */
void TranslationTableModel::clearTranslations()
{
    for (int i = 0; i < m_translations.size(); i++)
        m_translations[createIndex(i, 1).row()][Column(TRANSLATION)] = "";
    emit dataChanged(createIndex(0, 1), createIndex(m_translations.size() - 1, 1));
}

/*!
 * \brief Sets translations list for translations table.
 * \param translations List of translations.
 */
void TranslationTableModel::setTranslations(const QList<TranslationLocation> &translations)
{
    m_translations = translations;
    emit dataChanged(createIndex(0, 0), createIndex(translations.size() - 1, 1));
}

/*!
  * \brief Return list of application's name translations.
*/
QList<TranslationTableModel::TranslationLocation> TranslationTableModel::getTranslations() const
{
    return m_translations;
}

/*!
 * \brief Returns list of translations with languages.
 */
QList<TranslationData> TranslationTableModel::getTranslationData() const
{
    TranslationData td;
    QList<TranslationData> tdl;
    for (int i = 0; i < m_translations.count(); i++) {
        TranslationLocation tl = m_translations.at(i);
        td.setLanguage(tl[LANGUAGE].toString().simplified());
        td.setTranslation(tl[TRANSLATION].toString().simplified());
        tdl.append(td);
    }
    return tdl;
}

TranslationData::TranslationData()
{
    fillLanguageCodeMap();
}

TranslationData::TranslationData(const QString &language, const QString &translation)
    : m_language(language)
    , m_translation(translation)
{
    fillLanguageCodeMap();
}

void TranslationData::fillLanguageCodeMap()
{
    for (int i = 0; i < m_languagesList.count(); i++)
        m_languageCodeMap.insert(m_languagesList.at(i), m_languageCodesList.at(i));
}

QStringList TranslationData::getLanguages() const
{
    return m_languagesList;
}

QStringList TranslationData::getLanguageCodes() const
{
    return m_languageCodesList;
}

/*!
 * \brief Returns list of languages with their codes.
 */
QMap<QString, QString> TranslationData::getLanguageCodeMap() const
{
    return m_languageCodeMap;
}

/*!
 * \brief The constructor of the settings page
 * sets the user interface and the translations table.
 * \param defaultPath Path to the project.
 * \param langTableModel Model of translations table.
 * \param parent The parent object instance.
 */
DesktopSettingPage::DesktopSettingPage(const QString &defaultPath, TranslationTableModel *langTableModel,
                                       QWidget *parent)
    : QWizardPage (parent)
{
    m_settingUi.setupUi(this);
    m_settingUi.appPathEdit->setPlaceholderText("/usr/bin/" + QDir(defaultPath).dirName());
    m_langTableModel = langTableModel;
    langTableModel->setLanguageList(TranslationData().getLanguages());

    m_settingUi.langTableView->setModel(langTableModel);
    langTableModel->clearTranslations();

    QObject::connect(this, &DesktopSettingPage::translationsCleaned, langTableModel,
                     &TranslationTableModel::clearTranslations);

    registerField("appName*", m_settingUi.appNameEdit);
    registerField("appPath*", m_settingUi.appPathEdit);
    registerField("privileges", m_settingUi.checkPrivileges);
    registerField("translations", m_settingUi.langTableView);
}

/*!
 * \brief Destructor clears the memory of model object
 * after the creation of the file.
 */
DesktopSettingPage::~DesktopSettingPage()
{
    delete m_langTableModel;
}

/*!
 * \brief Cleanup fields of page.
 */
void DesktopSettingPage::cleanupPage()
{
    m_settingUi.appNameEdit->clear();
    m_settingUi.appPathEdit->clear();
    m_settingUi.checkPrivileges->setChecked(false);
    m_settingUi.langTableView->clearSelection();
    emit translationsCleaned();
}

/*!
 * \brief This is a override function from the \c QWizardPage, responsible for moving to the next page.
 * Returns \c true if the input fields are validated; otherwise  \c false.
 */
bool DesktopSettingPage::isComplete() const
{
    return DesktopLocationPage::validateInput(!field("appName").toString().isEmpty(),
                                              m_settingUi.appNameEdit,
                                              m_settingUi.errorLabel, tr("The application name line should not be empty"))
           && DesktopLocationPage::validateInput(!field("appPath").toString().isEmpty(),
                                                 m_settingUi.appPathEdit,
                                                 m_settingUi.errorLabel, tr("The execution command line should not be empty"));
}

} // namespace Internal
} // namespace SailfishWizards
