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

#include "spectaclefileeditorwidget.h"
#include "common.h"
#include <QCheckBox>
#include <QDebug>

namespace SailfishWizards {
namespace Internal {

/*!
 * \brief Constructor for the spectacle editor widget. Sets the interface by creating spectacle yaml file wizard's pages and list models.
 * \param parent Parent object instance.
 * \sa SpectacleFileOptionsPage, SpectacleFileProjectTypePage, SpectacleFileDependenciesPage, DependencyListModel
 */
SpectacleFileEditorWidget::SpectacleFileEditorWidget(QWidget *parent)
    : QTabWidget(parent)
{
    m_dependencyLists = new QList<DependencyListModel *>({new DependencyListModel, new DependencyListModel, new DependencyListModel});
    m_optionsPage = new SpectacleFileOptionsPage("", this);
    m_dependenciesPage = new SpectacleFileProjectTypePage(m_dependencyLists, this);
    m_dependenciesExpertPage = new SpectacleFileDependenciesPage(m_dependencyLists, this);
    m_optionsPage->Ui().summaryLineEdit->clear();
    m_optionsPage->Ui().urlLineEdit->clear();
    m_optionsPage->Ui().versionLineEdit->clear();
    m_optionsPage->Ui().descTextEdit->clear();
    m_optionsPage->Ui().licenseComboBox->setCurrentIndex(m_optionsPage->Ui().licenseComboBox->count() -
                                                         1);
    m_optionsPage->Ui().licenseLineEdit->clear();
    m_dependenciesPage->Ui().dependenciesDialogButton->setVisible(false);

    m_warningBox = new QMessageBox(m_dependenciesExpertPage);
    m_warningBox->setCheckBox(new QCheckBox(m_warningBox));
    m_warningBox->setText(tr("This page is for experts only. DO NOT use it, "
                             "if you don't know the exact low-level dependencies you need."));
    m_warningBox->checkBox()->setText(tr("Don't show this again."));
    m_warningBox->setIcon(QMessageBox::Icon::Information);

    addTab(m_optionsPage, tr("Package description"));
    addTab(m_dependenciesPage, tr("Project components"));
    addTab(m_dependenciesExpertPage, tr("System package dependencies"));

    QObject::connect(this, &SpectacleFileEditorWidget::currentChanged, this,
                     &SpectacleFileEditorWidget::showWarning);

    QObject::connect(m_optionsPage->Ui().licenseLineEdit, &QLineEdit::textChanged, this, &SpectacleFileEditorWidget::contentModified);
    QObject::connect(m_optionsPage->Ui().summaryLineEdit, &QLineEdit::textChanged, this, &SpectacleFileEditorWidget::contentModified);
    QObject::connect(m_optionsPage->Ui().versionLineEdit, &QLineEdit::textChanged, this, &SpectacleFileEditorWidget::contentModified);

    QObject::connect(m_optionsPage->Ui().urlLineEdit, &QLineEdit::textChanged, this, &SpectacleFileEditorWidget::contentModified);
    QObject::connect(m_optionsPage->Ui().descTextEdit, &QPlainTextEdit::textChanged, this, &SpectacleFileEditorWidget::contentModified);
    QObject::connect(m_dependenciesPage, &SpectacleFileProjectTypePage::listsModified, this, &SpectacleFileEditorWidget::contentModified);
    QObject::connect(m_dependenciesExpertPage, &SpectacleFileDependenciesPage::listsModified, this, &SpectacleFileEditorWidget::contentModified);

    QObject::connect(m_dependenciesExpertPage, &SpectacleFileDependenciesPage::listsModified, this,
                     &SpectacleFileEditorWidget::setChecks);

}

/*!
 * \brief Returns text content of the file edited with the widget.
 */
QString SpectacleFileEditorWidget::content()
{
    QString strRequirements[3];
    if (!(*m_dependencyLists).isEmpty()) {
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < (*m_dependencyLists)[i]->size(); j++)
                strRequirements[i] += "  - " + (*m_dependencyLists)[i]->getDependencyList().takeAt(
                                          j).toString() + "\n";
            strRequirements[i].chop(1);
        }
    }
    QString saveContent = m_fileContent;
    saveContent.replace("%{summary}", m_optionsPage->Ui().summaryLineEdit->text());
    saveContent.replace("%{configversion}", m_optionsPage->Ui().versionLineEdit->text());
    saveContent.replace("%{url}", m_optionsPage->Ui().urlLineEdit->text());
    saveContent.replace("%{license}", m_optionsPage->Ui().licenseLineEdit->text());
    saveContent.replace("%{description}", m_optionsPage->Ui().descTextEdit->toPlainText().replace("\n",
                                                                                                  "\n  ").trimmed());
    saveContent.replace("%{pkgbr}", strRequirements[PKG_BR]);
    saveContent.replace("%{pkgconfigbr}", strRequirements[PKG_CONFIG_BR]);
    saveContent.replace("%{requires}", strRequirements[REQUIRES]);
    return saveContent;
}

/*!
 * \brief Parses file's \a content and initializes widget's fields.
 * \param content Text content of the file.
 */
void SpectacleFileEditorWidget::setContent(const QByteArray &content)
{
    m_fileContent = "";
    QList<QString> notExistingTags = {"Summary", "Version", "URL", "License", "Description", "PkgBR", "PkgConfigBR", "Requires"};
    QTextStream input(content);
    QString line;
    bool previous_line = false;
    while (!input.atEnd() || !line.isEmpty()) {
        if (!previous_line)
            line = input.readLine();
        else
            previous_line = false;
        if (line.startsWith("#")) {
            m_fileContent += line + "\n";
            continue;
        }
        if ((line.contains(":")) && (!line.contains(" #") || line.indexOf(":") < line.indexOf(" #"))) {
            QString value = line.split(": ").last().split(" #").first().trimmed();
            QString tag = line.split(":").first();
            notExistingTags.removeOne(tag);
            if (tag == "Summary") {
                m_optionsPage->Ui().summaryLineEdit->setText(value);
                line.replace(line.indexOf(value), value.size(), "%{summary}");
            }
            if (tag == "Version") {
                m_optionsPage->Ui().versionLineEdit->setText(value);
                line.replace(line.indexOf(value), value.size(), "%{configversion}");
            }
            if (tag == "URL") {
                m_optionsPage->Ui().urlLineEdit->setText(value);
                line.replace(line.indexOf(value), value.size(), "%{url}");
            }
            if (tag == "License") {
                QComboBox *licenseBox = m_optionsPage->Ui().licenseComboBox;
                licenseBox->setCurrentIndex(m_optionsPage->shortLicenses().count() - 1);
                m_optionsPage->Ui().licenseLineEdit->setText(value);
                for (int i = 0; i < m_optionsPage->fullLicenses().count(); ++i) {
                    if (value == m_optionsPage->fullLicenses()[i])
                        licenseBox->setCurrentIndex(i);
                }
                line.replace(line.indexOf(value), value.size(), "%{license}");
            }
            if (tag == "Description") {
                QString description;
                line = input.readLine();
                QString possibleTag = line.split(":").first();
                while (!line.startsWith(possibleTag + ": ") && line != possibleTag + ":"
                        && !line.isEmpty() && !line.startsWith("#")) {
                    description += line.trimmed() + "\n";
                    line = input.readLine();
                    possibleTag = line.split(":").first();
                }
                m_optionsPage->Ui().descTextEdit->setPlainText(description);
                m_fileContent += "Description: |\n  %{description}\n";
                previous_line = true;
            }
            if (tag == "PkgConfigBR") {
                line = input.readLine();
                while (line.trimmed().startsWith("-")) {
                    m_dependencyLists->value(PKG_CONFIG_BR)->appendRecord(DependencyRecord().fromString(line));
                    line = input.readLine();
                }
                m_fileContent += "PkgConfigBR:\n%{pkgconfigbr}\n";
                previous_line = true;
            }
            if (tag == "PkgBR") {
                line = input.readLine();
                while (line.trimmed().startsWith("-")) {
                    m_dependencyLists->value(PKG_BR)->appendRecord(DependencyRecord().fromString(line));
                    line = input.readLine();
                }
                m_fileContent += "PkgBR:\n%{pkgbr}\n";
                previous_line = true;
            }
            if (tag == "Requires") {
                line = input.readLine();
                while (line.trimmed().startsWith("-")) {
                    m_dependencyLists->value(REQUIRES)->appendRecord(DependencyRecord().fromString(line));
                    line = input.readLine();
                }
                m_fileContent += "Requires:\n%{requires}\n";
                previous_line = true;
            }
        }
        if (!previous_line) m_fileContent += line + "\n";
    }
    for (QString tag : notExistingTags) {
        QString space;
        if (tag == "PkgBR" || tag == "PkgConfigBR" || tag == "Requires")
            space = "\n";
        else
            space = " ";
        m_fileContent += tag + ":" + space + "%{" + (tag == "Version" ? "config" : "") + tag.toLower() +
                         "}\n";
    }
    setChecks();
}

/*!
 * \brief Initializes checks on the project component page according to the file's dependency lists.
 */
void SpectacleFileEditorWidget::setChecks()
{
    QList<ProjectComponent> jsonLists = m_dependenciesPage->getJsonLists();
    for (ProjectComponent component : jsonLists) {
        bool check = true;
        QList<DependencyListModel *> dependencies = component.getDependencies();
        for (int i = 0; i < dependencies.count(); ++i) {
            for (DependencyRecord record : dependencies.value(i)->getDependencyList())
                if (!m_dependencyLists->value(i)->getDependencyList().contains(record)) {
                    check = false;
                    break;
                }
            if (!check)
                break;
        }
        CheckComponentTableModel *model = m_dependenciesPage->getTableModel();
        model->blockSignals(true);
        model->checkByName(component.getName(), check);
        model->blockSignals(false);
    }
}

/*!
 * \brief Shows warning window when expert page selected.
 * \param index Index of the selected page.
 */
void SpectacleFileEditorWidget::showWarning(int index)
{
    if (index == indexOf(m_dependenciesExpertPage) && !m_warningBox->checkBox()->checkState())
        m_warningBox->exec();
}

/*!
 * \brief Removes all of the dependencies from dependency lists.
 */
void SpectacleFileEditorWidget::clearDependencies()
{
    for (int i = 0; i < m_dependencyLists->count(); ++i)
        m_dependencyLists->value(i)->clearRecords();
}

} // namespace Internal
} // namespace SailfishWizards
