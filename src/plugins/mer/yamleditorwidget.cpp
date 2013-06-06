/****************************************************************************
**
** Copyright (C) 2012 - 2013 Jolla Ltd.
** Contact: http://jolla.com/
**
** This file is part of Qt Creator.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Digia.
**
****************************************************************************/

#include "yamleditorwidget.h"
#include "ui_yamleditorwidget.h"
#include "yamleditor.h"

#include <coreplugin/editormanager/ieditor.h>

namespace Mer {
namespace Internal {

YamlEditorWidget::YamlEditorWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::YamlEditorWidget),
    m_editor(0),
    m_modified(false),
    m_trackChanges(true)
{
    ui->setupUi(this);
    connect(ui->nameLineEdit, SIGNAL(textChanged(QString)), SLOT(onModified()));
    connect(ui->summaryLineEdit, SIGNAL(textChanged(QString)), SLOT(onModified()));
    connect(ui->versionLineEdit, SIGNAL(textChanged(QString)), SLOT(onModified()));
    connect(ui->releaseLineEdit, SIGNAL(textChanged(QString)), SLOT(onModified()));
    connect(ui->groupLineEdit, SIGNAL(textChanged(QString)), SLOT(onModified()));
    connect(ui->licenseTextedit, SIGNAL(textChanged()), SLOT(onModified()));
    connect(ui->descriptionTextEdit, SIGNAL(textChanged()), SLOT(onModified()));

    connect(ui->sourcesAddButton, SIGNAL(clicked()), SLOT(onModified()));
    connect(ui->sourcesRemoveButton, SIGNAL(clicked()), SLOT(onModified()));
    connect(ui->pkgConfigBRAddButton, SIGNAL(clicked()), SLOT(onModified()));
    connect(ui->pkgConfigBRRemoveButton, SIGNAL(clicked()), SLOT(onModified()));
    connect(ui->pkgBRAddButton, SIGNAL(clicked()), SLOT(onModified()));
    connect(ui->pkgBRRemoveButton, SIGNAL(clicked()), SLOT(onModified()));
    connect(ui->requiresAddButton, SIGNAL(clicked()), SLOT(onModified()));
    connect(ui->requiresRemoveButton, SIGNAL(clicked()), SLOT(onModified()));
    connect(ui->filesAddButton, SIGNAL(clicked()), SLOT(onModified()));
    connect(ui->filesRemoveButton, SIGNAL(clicked()), SLOT(onModified()));
}

YamlEditorWidget::~YamlEditorWidget()
{
    delete ui;
}

Core::IEditor *YamlEditorWidget::editor() const
{
    if (!m_editor) {
        m_editor = const_cast<YamlEditorWidget *>(this)->createEditor();
        connect(this, SIGNAL(changed()), m_editor, SIGNAL(changed()));
    }

    return m_editor;
}

QString YamlEditorWidget::name() const
{
    return ui->nameLineEdit->text();
}

void YamlEditorWidget::setName(const QString &name)
{
    ui->nameLineEdit->setText(name);
}

QString YamlEditorWidget::summary() const
{
    return ui->summaryLineEdit->text();
}

void YamlEditorWidget::setSummary(const QString &summary)
{
    ui->summaryLineEdit->setText(summary);
}

QString YamlEditorWidget::version() const
{
    return ui->versionLineEdit->text();
}

void YamlEditorWidget::setVersion(const QString &version)
{
    ui->versionLineEdit->setText(version);
}

QString YamlEditorWidget::release() const
{
    return ui->releaseLineEdit->text();
}

void YamlEditorWidget::setRelease(const QString &license)
{
    ui->releaseLineEdit->setText(license);
}

QString YamlEditorWidget::group() const
{
    return ui->groupLineEdit->text();
}

void YamlEditorWidget::setGroup(const QString &group)
{
    ui->groupLineEdit->setText(group);
}

QString YamlEditorWidget::license() const
{
    return ui->licenseTextedit->toPlainText();
}

void YamlEditorWidget::setLicense(const QString &license)
{
    ui->licenseTextedit->setPlainText(license);
}

QStringList YamlEditorWidget::sources() const
{
    QStringList sources;
    int count = ui->sourcesListWidget->count();
    while (count)
        sources << ui->sourcesListWidget->item(--count)->text();
    return sources;
}

void YamlEditorWidget::setSources(const QStringList &sources)
{
    ui->sourcesListWidget->clear();
    foreach (const QString &source, sources)
        ui->sourcesListWidget->addItem(source);
}

QString YamlEditorWidget::description() const
{
    return ui->descriptionTextEdit->toPlainText();
}

void YamlEditorWidget::setDescription(const QString &description)
{
    ui->descriptionTextEdit->setPlainText(description);
}

QStringList YamlEditorWidget::pkgConfigBR() const
{
    QStringList pkgConfigs;
    int count = ui->pkgConfigBRListWidget->count();
    while (count)
        pkgConfigs << ui->pkgConfigBRListWidget->item(--count)->text();
    return pkgConfigs;
}

void YamlEditorWidget::setPkgConfigBR(const QStringList &requires)
{
    ui->pkgConfigBRListWidget->clear();
    foreach (const QString &r, requires)
        ui->pkgConfigBRListWidget->addItem(r);
}

QStringList YamlEditorWidget::pkgBR() const
{
    QStringList packages;
    int count = ui->pkgBRListWidget->count();
    while (count)
        packages << ui->pkgBRListWidget->item(--count)->text();
    return packages;
}

void YamlEditorWidget::setPkgBR(const QStringList &packages)
{
    ui->pkgBRListWidget->clear();
    foreach (const QString &p, packages)
        ui->pkgBRListWidget->addItem(p);
}

QStringList YamlEditorWidget::requires() const
{
    QStringList requires;
    int count = ui->requiresListWidget->count();
    while (count)
        requires << ui->requiresListWidget->item(--count)->text();
    return requires;
}

void YamlEditorWidget::setRequires(const QStringList &requires)
{
    ui->requiresListWidget->clear();
    foreach (const QString &r, requires)
        ui->requiresListWidget->addItem(r);
}

QStringList YamlEditorWidget::files() const
{
    QStringList files;
    int count = ui->filesListWidget->count();
    while (count)
        files << ui->filesListWidget->item(--count)->text();
    return files;
}

void YamlEditorWidget::setFiles(const QStringList &files)
{
    ui->filesListWidget->clear();
    foreach (const QString &f, files)
        ui->filesListWidget->addItem(f);
}

bool YamlEditorWidget::modified() const
{
    return m_modified;
}

void YamlEditorWidget::setModified(bool modified)
{
    m_modified = modified;
    emit changed();
}

void YamlEditorWidget::setTrackChanges(bool track)
{
    m_trackChanges = track;
}

YamlEditor *YamlEditorWidget::createEditor()
{
    return new YamlEditor(this);
}

void YamlEditorWidget::onModified()
{
    if (m_trackChanges)
        setModified(true);
}

} // Internal
} // Mer
