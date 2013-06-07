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

#include "yamldocument.h"
#include "yamleditorwidget.h"
#include "merconstants.h"

#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/editormanager/editormanager.h>

#include <utils/textfileformat.h>
#include <mer/yaml/libyaml-stable/include/yaml.h>

#include <QFileInfo>
#include <QDir>
#include <QStack>
#include <QDebug>

namespace Mer {
namespace Internal {

class YamlDocumentPrivate
{
public:
    void fetchValues(const yaml_node_t *node);
    void setValues(yaml_node_t *node, bool set = false);
    bool isValidKey() const;

    QString fileName;
    YamlEditorWidget *editorWidget;

    yaml_parser_t yamlParser;
    yaml_emitter_t yamlWriter;
    yaml_document_t yamlDocument;
    bool success;

    QStack<QByteArray> valueStack;
};

YamlDocument::YamlDocument(YamlEditorWidget *parent) :
    Core::IDocument(parent),
    d(new YamlDocumentPrivate)
{
    d->editorWidget = parent;
}

YamlDocument::~YamlDocument()
{
    delete d;
}

bool YamlDocument::open(QString *errorString, const QString &fileName)
{
    d->success = yaml_parser_initialize(&d->yamlParser);
    if (d->success) {
        FILE *yamlFile = fopen(fileName.toUtf8(), "rb");
        if (yamlFile)
            yaml_parser_set_input_file(&d->yamlParser, yamlFile);
        d->success = yaml_parser_load(&d->yamlParser, &d->yamlDocument);

        d->fileName = fileName;
        d->editorWidget->editor()->setDisplayName(QFileInfo(fileName).fileName());

        if (d->success) {
            d->editorWidget->setTrackChanges(false);
            d->fetchValues(yaml_document_get_root_node(&d->yamlDocument));
            d->valueStack.clear();
            d->editorWidget->setTrackChanges(true);
        }
        fclose(yamlFile);
    }
    if (!d->success && errorString)
        *errorString = tr("Error %1").arg(QString::fromUtf8(d->yamlParser.problem));
    yaml_parser_delete(&d->yamlParser);
    return d->success;
}

bool YamlDocument::save(QString *errorString, const QString &fileName, bool autoSave)
{
    if (autoSave)
        return false;
    const QString actualName = fileName.isEmpty() ? this->fileName() : fileName;
    d->success = yaml_emitter_initialize(&d->yamlWriter);
    if (d->success) {
        d->setValues(yaml_document_get_root_node(&d->yamlDocument));
        d->valueStack.clear();
        d->editorWidget->setModified(false);

        FILE *yamlFile = fopen(actualName.toUtf8(), "wb");
        if (yamlFile)
            yaml_emitter_set_output_file(&d->yamlWriter, yamlFile);

        d->success = yaml_emitter_open(&d->yamlWriter);
        if (d->success)
            d->success = yaml_emitter_dump(&d->yamlWriter, &d->yamlDocument);
        if (d->success)
            d->success = yaml_emitter_close(&d->yamlWriter);
        if (d->success)
            d->success = yaml_emitter_flush(&d->yamlWriter);
        fclose(yamlFile);
    }
    if (!d->success && errorString)
        *errorString = tr("Error %1").arg(QString::fromUtf8(d->yamlWriter.problem));
    yaml_emitter_delete(&d->yamlWriter);
    if (d->success)
        open(0, actualName);
    return d->success;
}

QString YamlDocument::fileName() const
{
    return d->fileName;
}

QString YamlDocument::defaultPath() const
{
    QFileInfo fi(fileName());
    return fi.absolutePath();
}

QString YamlDocument::suggestedFileName() const
{
    QFileInfo fi(fileName());
    return fi.fileName();
}

QString YamlDocument::mimeType() const
{
    return QLatin1String(Constants::MER_YAML_MIME_TYPE);
}

bool YamlDocument::isModified() const
{
    return d->editorWidget->modified();
}

bool YamlDocument::isSaveAsAllowed() const
{
    return false;
}

bool YamlDocument::reload(QString *errorString, Core::IDocument::ReloadFlag flag, Core::IDocument::ChangeType type)
{
    Q_UNUSED(type);

    if (flag == Core::IDocument::FlagIgnore)
        return true;

    return open(errorString, d->fileName);
}

void YamlDocument::rename(const QString &newName)
{
    const QString oldFilename = d->fileName;
    d->fileName = newName;
    d->editorWidget->editor()->setDisplayName(QFileInfo(d->fileName).fileName());
    emit fileNameChanged(oldFilename, newName);
    emit changed();
}

void YamlDocumentPrivate::fetchValues(const yaml_node_t *node)
{
    if (node) {
        switch (node->type) {
        case YAML_NO_NODE:
            break;
        case YAML_SCALAR_NODE:
            valueStack.push((char*)node->data.scalar.value);
            break;
        case YAML_SEQUENCE_NODE: {
            yaml_node_item_t *current = node->data.sequence.items.start;
            yaml_node_item_t *end = node->data.sequence.items.top;
            while (current && current < end) {
                yaml_node_t *nodeItem = yaml_document_get_node(&yamlDocument, *current);
                fetchValues(nodeItem);
                ++current;
            }
            break;
        }
        case YAML_MAPPING_NODE: {
            yaml_node_pair_t *current = node->data.mapping.pairs.start;
            yaml_node_pair_t *end = node->data.mapping.pairs.top;
            while (current && current < end) {
                yaml_node_t *nodeKey = yaml_document_get_node(&yamlDocument, current->key);
                fetchValues(nodeKey);
                if (!isValidKey()) {
                    valueStack.clear();
                    ++current;
                    continue;
                }
                const QByteArray key = valueStack.pop();
                yaml_node_t *nodeValue = yaml_document_get_node(&yamlDocument, current->value);
                fetchValues(nodeValue);
                if (key == "Name") {
                    editorWidget->setName(QString::fromUtf8(valueStack.pop()));
                } else if (key == "Summary") {
                    editorWidget->setSummary(QString::fromUtf8(valueStack.pop()));
                } else if (key == "Version") {
                    editorWidget->setVersion(QString::fromUtf8(valueStack.pop()));
                } else if (key == "Release") {
                    editorWidget->setRelease(QString::fromUtf8(valueStack.pop()));
                } else if (key == "Group") {
                    editorWidget->setGroup(QString::fromUtf8(valueStack.pop()));
                } else if (key == "License") {
                    editorWidget->setLicense(QString::fromUtf8(valueStack.pop()));
                } else if (key == "Sources") {
                    QStringList sources;
                    while (!valueStack.isEmpty())
                        sources << QString::fromUtf8(valueStack.pop());
                    editorWidget->setSources(sources);
                } else if (key == "Description") {
                    editorWidget->setDescription(QString::fromUtf8(valueStack.pop()));
                } else if (key == "PkgConfigBR") {
                    QStringList pkgConfigs;
                    while (!valueStack.isEmpty())
                        pkgConfigs << QString::fromUtf8(valueStack.pop());
                    editorWidget->setPkgConfigBR(pkgConfigs);
                } else if (key == "PkgBR") {
                    QStringList pkgs;
                    while (!valueStack.isEmpty())
                        pkgs << QString::fromUtf8(valueStack.pop());
                    editorWidget->setPkgBR(pkgs);
                } else if (key == "Requires") {
                    QStringList requires;
                    while (!valueStack.isEmpty())
                        requires << QString::fromUtf8(valueStack.pop());
                    editorWidget->setRequires(requires);
                } else if (key == "Files") {
                    QStringList files;
                    while (!valueStack.isEmpty())
                        files << QString::fromUtf8(valueStack.pop());
                    editorWidget->setFiles(files);
                }
                ++current;
            }
            break;
        }
        }
    }
}

void YamlDocumentPrivate::setValues(yaml_node_t *node, bool set)
{
    if (node) {
        switch (node->type) {
        case YAML_NO_NODE:
            break;
        case YAML_SCALAR_NODE:
            if (!set) {
                valueStack.push((char*)node->data.scalar.value);
            } else {
                free(node->data.scalar.value);
                QByteArray value = valueStack.pop();
                int size = value.size();
                node->data.scalar.value = (yaml_char_t*)malloc(sizeof(unsigned char) * size + 1);
                node->data.scalar.length = size;
                memcpy(node->data.scalar.value, value.data(), size + 1);
            }
            break;
        case YAML_SEQUENCE_NODE: {
            yaml_node_item_t *current = node->data.sequence.items.start;
            yaml_node_item_t *end = node->data.sequence.items.top;
            while (current && current < end) {
                yaml_node_t *nodeItem = yaml_document_get_node(&yamlDocument, *current);
                setValues(nodeItem);
                ++current;
            }
            break;
        }
        case YAML_MAPPING_NODE: {
            yaml_node_pair_t *current = node->data.mapping.pairs.start;
            yaml_node_pair_t *end = node->data.mapping.pairs.top;
            while (current && current < end) {
                yaml_node_t *nodeKey = yaml_document_get_node(&yamlDocument, current->key);
                setValues(nodeKey);
                if (!isValidKey()) {
                    valueStack.clear();
                    ++current;
                    continue;
                }
                const QByteArray key = valueStack.pop();
                if (key == "Name") {
                    valueStack.push(editorWidget->name().toUtf8());
                } else if (key == "Summary") {
                    valueStack.push(editorWidget->summary().toUtf8());
                } else if (key == "Version") {
                    valueStack.push(editorWidget->version().toUtf8());
                } else if (key == "Release") {
                    valueStack.push(editorWidget->release().toUtf8());
                } else if (key == "Group") {
                    valueStack.push(editorWidget->group().toUtf8());
                } else if (key == "License") {
                    valueStack.push(editorWidget->license().toUtf8());
                } else if (key == "Sources") {
                    QStringList sources = editorWidget->sources();
                    foreach (const QString &source, sources)
                        valueStack.push_back(source.toUtf8());
                } else if (key == "Description") {
                    valueStack.push(editorWidget->description().toUtf8());
                } else if (key == "PkgConfigBR") {
                    QStringList pkgConfigs = editorWidget->pkgConfigBR();
                    foreach (const QString &pc, pkgConfigs)
                        valueStack.push_back(pc.toUtf8());
                } else if (key == "PkgBR") {
                    QStringList pkgs = editorWidget->pkgBR();
                    foreach (const QString &p, pkgs)
                        valueStack.push_back(p.toUtf8());
                } else if (key == "Requires") {
                    QStringList requires = editorWidget->requires();
                    foreach (const QString &r, requires)
                        valueStack.push_back(r.toUtf8());
                } else if (key == "Files") {
                    QStringList files = editorWidget->files();
                    foreach (const QString &f, files)
                        valueStack.push_back(f.toUtf8());
                }
                yaml_node_t *nodeValue = yaml_document_get_node(&yamlDocument, current->value);
                setValues(nodeValue, true);
                ++current;
            }
            break;
        }
        }
    }
}

bool YamlDocumentPrivate::isValidKey() const
{
    if (valueStack.isEmpty())
        return false;
    const QByteArray key = valueStack.top();

    return key == "Name" || key == "Summary" || key == "Version" || key == "Release"
            || key == "Group" || key == "License" || key == "Sources" || key == "Description"
            || key == "PkgConfigBR" || key == "PkgBR" || key == "Requires" || key == "Files";
}

} // Internal
} // Mer
