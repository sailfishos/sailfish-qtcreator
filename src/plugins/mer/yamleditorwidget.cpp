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

#include <QFileDialog>

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

    connect(ui->sourcesAddButton, SIGNAL(clicked()), SLOT(onAddItem()));
    connect(ui->sourcesRemoveButton, SIGNAL(clicked()), SLOT(onRemoveItem()));
    connect(ui->pkgConfigBRAddButton, SIGNAL(clicked()), SLOT(onAddItem()));
    connect(ui->pkgConfigBRRemoveButton, SIGNAL(clicked()), SLOT(onRemoveItem()));
    connect(ui->pkgBRAddButton, SIGNAL(clicked()), SLOT(onAddItem()));
    connect(ui->pkgBRRemoveButton, SIGNAL(clicked()), SLOT(onRemoveItem()));
    connect(ui->requiresAddButton, SIGNAL(clicked()), SLOT(onAddItem()));
    connect(ui->requiresRemoveButton, SIGNAL(clicked()), SLOT(onRemoveItem()));
    connect(ui->filesAddButton, SIGNAL(clicked()), SLOT(onAddFiles()));
    connect(ui->filesRemoveButton, SIGNAL(clicked()), SLOT(onRemoveFiles()));

    connect(ui->sourcesListWidget, SIGNAL(itemActivated(QListWidgetItem*)),
            SLOT(onItemActivated(QListWidgetItem*)));
    connect(ui->sourcesListWidget, SIGNAL(itemChanged(QListWidgetItem*)),
            SLOT(onModified()));
    connect(ui->sourcesListWidget, SIGNAL(itemSelectionChanged()),
            SLOT(onSelectionChanged()));
    connect(ui->pkgConfigBRListWidget, SIGNAL(itemActivated(QListWidgetItem*)),
            SLOT(onItemActivated(QListWidgetItem*)));
    connect(ui->pkgConfigBRListWidget, SIGNAL(itemChanged(QListWidgetItem*)),
            SLOT(onModified()));
    connect(ui->pkgConfigBRListWidget, SIGNAL(itemSelectionChanged()),
            SLOT(onSelectionChanged()));
    connect(ui->pkgBRListWidget, SIGNAL(itemActivated(QListWidgetItem*)),
            SLOT(onItemActivated(QListWidgetItem*)));
    connect(ui->pkgBRListWidget, SIGNAL(itemChanged(QListWidgetItem*)),
            SLOT(onModified()));
    connect(ui->pkgBRListWidget, SIGNAL(itemSelectionChanged()),
            SLOT(onSelectionChanged()));
    connect(ui->requiresListWidget, SIGNAL(itemActivated(QListWidgetItem*)),
            SLOT(onItemActivated(QListWidgetItem*)));
    connect(ui->requiresListWidget, SIGNAL(itemChanged(QListWidgetItem*)),
            SLOT(onModified()));
    connect(ui->requiresListWidget, SIGNAL(itemSelectionChanged()),
            SLOT(onSelectionChanged()));
    connect(ui->filesListWidget, SIGNAL(itemActivated(QListWidgetItem*)),
            SLOT(onItemActivated(QListWidgetItem*)));
    connect(ui->filesListWidget, SIGNAL(itemChanged(QListWidgetItem*)),
            SLOT(onModified()));
    connect(ui->filesListWidget, SIGNAL(itemSelectionChanged()),
            SLOT(onSelectionChanged()));

    ui->sourcesRemoveButton->setEnabled(false);
    ui->pkgConfigBRRemoveButton->setEnabled(false);
    ui->pkgBRRemoveButton->setEnabled(false);
    ui->requiresRemoveButton->setEnabled(false);
    ui->filesRemoveButton->setEnabled(false);

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
    while (count) {
        const QString text = ui->sourcesListWidget->item(--count)->text();
        if (text == tr("<New Item>"))
            continue;
        sources << text;
    }
    return sources;
}

void YamlEditorWidget::setSources(const QStringList &sources)
{
    ui->sourcesListWidget->clear();
    foreach (const QString &source, sources) {
        if (source.isEmpty())
            continue;
        QListWidgetItem *item = new QListWidgetItem(source);
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
        ui->sourcesListWidget->addItem(item);
    }
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
    while (count) {
        const QString text = ui->pkgConfigBRListWidget->item(--count)->text();
        if (text == tr("<New Item>"))
            continue;
        pkgConfigs << text;
    }
    return pkgConfigs;
}

void YamlEditorWidget::setPkgConfigBR(const QStringList &requires)
{
    ui->pkgConfigBRListWidget->clear();
    foreach (const QString &r, requires) {
        if (r.isEmpty())
            continue;
        QListWidgetItem *item = new QListWidgetItem(r);
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
        ui->pkgConfigBRListWidget->addItem(item);
    }
}

QStringList YamlEditorWidget::pkgBR() const
{
    QStringList packages;
    int count = ui->pkgBRListWidget->count();
    while (count) {
        const QString text = ui->pkgBRListWidget->item(--count)->text();
        if (text == tr("<New Item>"))
            continue;
        packages << text;
    }
    return packages;
}

void YamlEditorWidget::setPkgBR(const QStringList &packages)
{
    ui->pkgBRListWidget->clear();
    foreach (const QString &p, packages) {
        if (p.isEmpty())
            continue;
        QListWidgetItem *item = new QListWidgetItem(p);
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
        ui->pkgBRListWidget->addItem(item);
    }
}

QStringList YamlEditorWidget::requires() const
{
    QStringList requires;
    int count = ui->requiresListWidget->count();
    while (count) {
        const QString text = ui->requiresListWidget->item(--count)->text();
        if (text == tr("<New Item>"))
            continue;
        requires << text;
    }
    return requires;
}

void YamlEditorWidget::setRequires(const QStringList &requires)
{
    ui->requiresListWidget->clear();
    foreach (const QString &r, requires) {
        if (r.isEmpty())
            continue;
        QListWidgetItem *item = new QListWidgetItem(r);
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
        ui->requiresListWidget->addItem(item);
    }
}

QStringList YamlEditorWidget::files() const
{
    QStringList files;
    int count = ui->filesListWidget->count();
    while (count) {
        const QString text = ui->filesListWidget->item(--count)->text();
        if (text == tr("<New Item>"))
            continue;
        files << text;
    }
    return files;
}

void YamlEditorWidget::setFiles(const QStringList &files)
{
    ui->filesListWidget->clear();
    foreach (const QString &f, files) {
        if (f.isEmpty())
            continue;
        QListWidgetItem *item = new QListWidgetItem(f);
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
        ui->filesListWidget->addItem(item);
    }
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

void YamlEditorWidget::clear()
{
    ui->nameLineEdit->clear();
    ui->summaryLineEdit->clear();
    ui->versionLineEdit->clear();
    ui->releaseLineEdit->clear();
    ui->groupLineEdit->clear();
    ui->licenseTextedit->clear();
    ui->descriptionTextEdit->clear();
    ui->sourcesListWidget->clear();
    ui->pkgBRListWidget->clear();
    ui->pkgConfigBRListWidget->clear();
    ui->requiresListWidget->clear();
    ui->filesListWidget->clear();
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

void YamlEditorWidget::onAddFiles()
{
    QStringList files = QFileDialog::getOpenFileNames(this, tr("Select files"));
    foreach (const QString &f, files) {
        QListWidgetItem *item = new QListWidgetItem(f);
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
        ui->filesListWidget->addItem(item);
    }
    if (files.count())
        onModified();
}

void YamlEditorWidget::onRemoveFiles()
{
    QList<QListWidgetItem *> selectedItems = ui->filesListWidget->selectedItems();
    foreach (QListWidgetItem *item, selectedItems) {
        ui->filesListWidget->removeItemWidget(item);
        delete item;
    }
    if (selectedItems.count())
        onModified();
}

void YamlEditorWidget::onAddItem()
{
    QListWidget *widget = 0;
    QObject *button = sender();
    if (button == ui->sourcesAddButton)
        widget = ui->sourcesListWidget;
    else if (button == ui->pkgBRAddButton)
        widget = ui->pkgBRListWidget;
    else if (button == ui->pkgConfigBRAddButton)
        widget = ui->pkgConfigBRListWidget;
    else if (button == ui->requiresAddButton)
        widget = ui->requiresListWidget;
    if (widget) {
        QListWidgetItem *item = new QListWidgetItem(tr("<New Item>"));
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
        item->setSelected(true);
        widget->addItem(item);
        onModified();
    }
}

void YamlEditorWidget::onRemoveItem()
{
    QListWidget *widget = 0;
    QPushButton *button = qobject_cast<QPushButton *>(sender());
    if (button == ui->sourcesRemoveButton)
        widget = ui->sourcesListWidget;
    else if (button == ui->pkgBRRemoveButton)
        widget = ui->pkgBRListWidget;
    else if (button == ui->pkgConfigBRRemoveButton)
        widget = ui->pkgConfigBRListWidget;
    else if (button == ui->requiresRemoveButton)
        widget = ui->requiresListWidget;
    if (widget) {
        QList<QListWidgetItem *> selectedItems = widget->selectedItems();
        foreach (QListWidgetItem *item, selectedItems) {
            ui->filesListWidget->removeItemWidget(item);
            delete item;
        }
        if (selectedItems.count())
            onModified();
    }
}

void YamlEditorWidget::onItemActivated(QListWidgetItem *item)
{
    QListWidget *widget = qobject_cast<QListWidget *>(sender());
    if (widget)
        widget->editItem(item);
}

void YamlEditorWidget::onSelectionChanged()
{
    QListWidget *widget = qobject_cast<QListWidget *>(sender());
    QPushButton *button = 0;
    if (widget == ui->sourcesListWidget)
        button = ui->sourcesRemoveButton;
    else if (widget == ui->pkgBRListWidget)
        button = ui->pkgBRRemoveButton;
    else if (widget == ui->pkgConfigBRListWidget)
        button = ui->pkgConfigBRRemoveButton;
    else if (widget == ui->requiresListWidget)
        button = ui->requiresRemoveButton;
    else if (widget == ui->filesListWidget)
        button = ui->filesRemoveButton;
    if (widget && button)
        button->setEnabled(widget->selectedItems().count());
}

} // Internal
} // Mer
