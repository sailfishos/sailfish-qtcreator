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

#include "desktopeditorwidget.h"
#include <QImageReader>

namespace SailfishWizards {
namespace Internal {

/*!
 * \brief Constructor for the desktop editor widget. Sets the interface by creating desktop wizard's pages and table model.
 * \param parent Parent object instance.
 * \sa DesktopSettingPage, DesktopIconPage, TranslationTableModel
 */
DesktopEditorWidget::DesktopEditorWidget(QWidget *parent)
    : QTabWidget (parent)
{
    m_tableModel = new TranslationTableModel(this);
    m_settingPage = new DesktopSettingPage("", m_tableModel, this);
    m_iconPage = new DesktopIconPage;
    m_settingPage->Ui().appPathEdit->setPlaceholderText(tr("Execution file path"));
    m_iconPage->Ui().icon86Path->installEventFilter(this);
    m_iconPage->Ui().icon108Path->installEventFilter(this);
    m_iconPage->Ui().icon128Path->installEventFilter(this);
    m_iconPage->Ui().icon172Path->installEventFilter(this);

    addTab(m_settingPage, tr("Options"));
    addTab(m_iconPage, tr("Icon"));
    QObject::connect(m_settingPage->Ui().appNameEdit, &QLineEdit::textChanged, this, &DesktopEditorWidget::contentModified);
    QObject::connect(m_settingPage->Ui().appPathEdit, &QLineEdit::textChanged, this, &DesktopEditorWidget::contentModified);
    QObject::connect(m_iconPage->Ui().iconPath, &QLineEdit::textChanged, this, &DesktopEditorWidget::contentModified);
    QObject::connect(m_iconPage->Ui().icon86Path, &QLineEdit::textChanged, this, &DesktopEditorWidget::contentModified);
    QObject::connect(m_iconPage->Ui().icon108Path, &QLineEdit::textChanged, this, &DesktopEditorWidget::contentModified);
    QObject::connect(m_iconPage->Ui().icon128Path, &QLineEdit::textChanged, this, &DesktopEditorWidget::contentModified);
    QObject::connect(m_iconPage->Ui().icon172Path, &QLineEdit::textChanged, this, &DesktopEditorWidget::contentModified);

    QObject::connect(m_settingPage->Ui().checkPrivileges, &QCheckBox::stateChanged, this, &DesktopEditorWidget::contentModified);
    QObject::connect(m_iconPage->Ui().manualInputCheck, &QCheckBox::stateChanged, this, &DesktopEditorWidget::contentModified);
    QObject::connect(m_tableModel, &TranslationTableModel::dataChanged, this, &DesktopEditorWidget::contentModified);
}

/*!
 * \brief Destructor for the editor widget.
 * Releases the allocated memory.
 */
DesktopEditorWidget::~DesktopEditorWidget()
{
//    delete m_tableModel;
    delete m_iconPage;
}

/*!
 * \brief This function describes and returns the event filter for the editor widget.
 * It is reimplemented for additional control of focusing events of widget's line edits, which is needed for rendering of the associated icon when line edit is selected.
 * \param target Object containing the information about the object with which the event occurred.
 * \param event Object containing all the event information.
 */
bool DesktopEditorWidget::eventFilter(QObject *target, QEvent *event)
{
    if (target == m_iconPage->Ui().icon86Path || target == m_iconPage->Ui().icon108Path
            || target == m_iconPage->Ui().icon128Path || target == m_iconPage->Ui().icon172Path) {
        if (event->type() == QEvent::FocusIn) {
            QLineEdit *lineEdit = static_cast<QLineEdit *>(target);
            QPixmap icon = QPixmap(lineEdit->text());
            if (icon.height() != 0) {
                m_iconPage->Ui().sizeLabel->setText(tr("Icon size:\n") + QString::number(icon.height()) + "x" +
                                                    QString::number(
                                                        icon.width()));
                m_iconPage->Ui().iconLabel->setPixmap(icon);
            }
        }
        if (event->type() == QEvent::FocusOut) {
            m_iconPage->Ui().sizeLabel->clear();
            m_iconPage->Ui().iconLabel->clear();
        }
    }
    return QTabWidget::eventFilter(target, event);
}

/*!
 * \brief Returns text content of the file edited with the widget.
 */
QString DesktopEditorWidget::content() const
{
    QStringList languages = TranslationData().getLanguages();
    QStringList languageCodes = TranslationData().getLanguageCodes();
    QList<TranslationTableModel::TranslationLocation> translations = m_tableModel->getTranslations();
    QString saveContent = m_fileContent;
    saveContent.replace("%{Name}", m_settingPage->Ui().appNameEdit->text());
    QString execPrefix = (m_settingPage->Ui().checkPrivileges->isChecked() ?
                          QString("invoker --type=silica-qt5 -s ") : "");
    saveContent.replace("%{Exec}", execPrefix + m_settingPage->Ui().appPathEdit->text());
    saveContent.replace("%{Terminal}", "false");
    saveContent.replace("%{Type}", "Application");
    for (TranslationTableModel::TranslationLocation translation : translations) {
        QString word = translation[TranslationTableModel::Column::TRANSLATION].toString();
        QString language = translation[TranslationTableModel::Column::LANGUAGE].toString();
        QString code = languageCodes[languages.indexOf(language)];
        if (!word.isEmpty()) {
            if (saveContent.contains("Name[" + code + "]"))
                saveContent.replace("%{Name[" + code + "]}", word);
            else
                saveContent += "Name[" + code + "]=" + word + "\n";
        } else {
            if (saveContent.contains("Name[" + code + "]")) {
                QStringList strings = saveContent.split("\n");
                for (QString string : strings)
                    if (string.contains("Name[" + code + "]"))
                        strings.removeOne(string);
                saveContent = strings.join("\n");
            }
        }
    }
    return saveContent;
}

/*!
 * \brief Parses file's \a content and initializes widget's fields.
 * \param content Text content of the file.
 * \param projectPath Path to the file's project directory.
 * \param iconResolutions List of supported icon resolutions.
 */
void DesktopEditorWidget::setContent(const QByteArray &content, const QString &projectPath,
                                     const QStringList &iconResolutions)
{
    const QStringList languages = TranslationData().getLanguages();
    const QStringList languageCodes = TranslationData().getLanguageCodes();
    m_fileContent = "";
    QTextStream input(content);
    QString line;
    while (!input.atEnd()) {
        line = input.readLine();
        if (line.contains("[Desktop Entry]")) {
            m_fileContent += "[Desktop Entry]";
            line.remove("[Desktop Entry]");
        }
        if (line.isEmpty()) {
            m_fileContent += "\n";
            continue;
        }
        QString key = line.left(line.indexOf("="));
        QString value = line.remove(0, key.length() + 1);
        m_fileContent += key + "=" + "%{" + key + "}\n";
        if (key == "Name")
            m_settingPage->Ui().appNameEdit->setText(value);
        if (key == "Exec") {
            if (value.trimmed().startsWith("invoker --type=silica-qt5 -s"))
                m_settingPage->Ui().checkPrivileges->setCheckState(Qt::CheckState::Checked);
            m_settingPage->Ui().appPathEdit->setText(value.remove("invoker --type=silica-qt5 -s").trimmed());
        }
        if (key == "Icon") {
            QDir projectDirectory = QDir(projectPath);
            projectDirectory.setNameFilters(QStringList() << value + ".png" << value + ".jpg" << value +
                                            ".bmp");
            if (!projectDirectory.entryList().isEmpty()) {
                m_iconPage->Ui().manualInputCheck->setCheckState(Qt::CheckState::Unchecked);
                m_iconPage->Ui().iconPath->setText(projectDirectory.path() + QDir::separator() +
                                                   projectDirectory.entryList().first());
            } else {
                m_iconPage->Ui().manualInputCheck->setCheckState(Qt::CheckState::Checked);
                QList<QLineEdit *> pathEdits = {m_iconPage->Ui().icon86Path, m_iconPage->Ui().icon108Path, m_iconPage->Ui().icon128Path, m_iconPage->Ui().icon172Path};
                for (int i = 0; i < pathEdits.length(); i++) {
                    QDir directory = QDir(projectPath + QDir::separator() + iconResolutions[i]);
                    directory.setNameFilters(QStringList() << value + ".png" << value + ".jpg" << value + ".bmp");
                    if (directory.exists() && !directory.entryList().isEmpty())
                        pathEdits[i]->setText(directory.path() + QDir::separator() + directory.entryList().first());
                }
            }
        }
        if (key.startsWith("Name[")) {
            QString language;
            key.remove("Name[");
            key.remove("]");
            if (languageCodes.contains(key))
                language = languages[languageCodes.indexOf(key)];
            else
                continue;
            QList<TranslationTableModel::TranslationLocation> translations = m_tableModel->getTranslations();
            for (TranslationTableModel::TranslationLocation &translation : translations)
                if (translation[TranslationTableModel::Column::LANGUAGE] == language)
                    translation[TranslationTableModel::Column::TRANSLATION] = value;
            m_tableModel->setTranslations(translations);
        }
    }
}

/*!
 * \brief Writes the icon name to the file. Name consists of the project name and index.
 * \param projectPath Path to the file's project directory.
 * \param iconsIndex Actual number of the icon set.
 * \sa DesktopDocument::countIconIndex()
 */
void DesktopEditorWidget::setIconValue(const QString &applicationPath, int iconsIndex)
{
    if (m_fileContent.contains("%{Icon}")) {
        m_fileContent.replace("%{Icon}",
                              QDir(applicationPath).dirName() + "_" + QString::number(iconsIndex));
    } else {
        if (m_fileContent.contains("Icon=")) {
            QStringList strings = m_fileContent.split("\n");
            for (QString &string : strings)
                if (string.contains("Icon="))
                    strings.replace(strings.indexOf(string),
                                    "Icon=" + QDir(applicationPath).dirName() + "_" + QString::number(iconsIndex));
            m_fileContent = strings.join("\n");
        } else {
            m_fileContent += "Icon=" + QDir(applicationPath).dirName() + "_" + QString::number(
                                        iconsIndex);
        }
    }
}

/*!
 * \brief Shows warning dialog about missing icon information. Deletes icon from file content if the user chooses to proceed.
 * Returns \c true if user agree with that, otherwise returns \c false.
 */
bool DesktopEditorWidget::proceedEmptyIcon()
{
    QMessageBox::StandardButton reply = QMessageBox::question(this, tr("Icon"),
                                                              tr("Your icon's information is not complete, icon will be removed from file. Continue?"),
                                                              QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        m_fileContent.remove("Icon=%{Icon}\n");
        return true;
    } else {
        return false;
    }
}

/*!
 * \brief Clears all entered data from the table model.
 */
void DesktopEditorWidget::clearTranslations()
{
    m_tableModel->clearTranslations();
}

/*!
 * \brief Returns list of the entered icon paths from the icon page.
 */
QHash<QString, QString> DesktopEditorWidget::iconPaths() const
{
    QHash<QString, QString> paths;
    paths.insert("General", m_iconPage->Ui().iconPath->text());
    paths.insert("86x86", m_iconPage->Ui().icon86Path->text());
    paths.insert("108x108", m_iconPage->Ui().icon108Path->text());
    paths.insert("128x128", m_iconPage->Ui().icon128Path->text());
    paths.insert("172x172", m_iconPage->Ui().icon172Path->text());
    return paths;
}

/*!
 * \brief Reloads icon page. Writes all specific icon paths.
 */
void DesktopEditorWidget::reloadIconPage(const QString &applicationPath, int iconsIndex)
{
    m_iconPage->Ui().iconPath->clear();
    m_iconPage->Ui().manualInputCheck->setCheckState(Qt::CheckState::Checked);
    m_iconPage->Ui().icon86Path->setText(applicationPath + "/86x86/" + QDir(
                                             applicationPath).dirName() +
                                         "_" + QString::number(iconsIndex) + ".png");
    m_iconPage->Ui().icon108Path->setText(applicationPath + "/108x108/" + QDir(
                                              applicationPath).dirName() +
                                          "_" + QString::number(iconsIndex) + ".png");
    m_iconPage->Ui().icon128Path->setText(applicationPath + "/128x128/" + QDir(
                                              applicationPath).dirName() +
                                          "_" + QString::number(iconsIndex) + ".png");
    m_iconPage->Ui().icon172Path->setText(applicationPath + "/172x172/" + QDir(
                                              applicationPath).dirName() +
                                          "_" + QString::number(iconsIndex) + ".png");
}

/*!
 * \brief Checks if the entered icons exist, have correct extension and resolution.
 * Shows error message if any of this conditions is not satisfied.
 * Returns \c true if all the validations passed, otherwise returns \c false.
 */
bool DesktopEditorWidget::validateIcons()
{
    QMimeDatabase mimeDB;
    QString errorString = "";
    if (m_iconPage->Ui().manualInputCheck->isChecked()) {

        QList<QPair<QString, QSize>> icons;
        icons.append(qMakePair(m_iconPage->Ui().icon86Path->text(), QSize(86, 86)));
        icons.append(qMakePair(m_iconPage->Ui().icon108Path->text(), QSize(108, 108)));
        icons.append(qMakePair(m_iconPage->Ui().icon128Path->text(), QSize(128, 128)));
        icons.append(qMakePair(m_iconPage->Ui().icon172Path->text(), QSize(172, 172)));

        for (const auto &iconInfo : icons) {
            const QString &iconPath = iconInfo.first;
            QImageReader iconReader(iconPath);
            if (!iconReader.canRead()) {
                errorString = tr("File \"") + iconPath +
                              tr("\" does not exist or is not a picture. Please, select existing png, jpg or bmp file.");
                break;
            }
            if (iconReader.size() != iconInfo.second) {
                errorString = tr("File \"") + iconPath + tr("\" does not have %1x%2 format.").arg(iconInfo.second.height()).arg(iconInfo.second.width());
                break;
            }
        }
    } else {
        const QString iconPath = m_iconPage->Ui().iconPath->text();
        QImageReader iconReader(iconPath);
        if (!iconReader.canRead())
            errorString = tr("File \"") + iconPath +
                          tr("\" does not exist or is not a picture. Please, select existing png, jpg or bmp file.");

    }
    if (!errorString.isEmpty()) {
        QMessageBox::information(this, tr("Icon"), errorString);
        return false;
    }
    return true;
}

} // namespace Internal
} // namespace SailfishWizards
