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
    YamlDocumentPrivate()
    {
        allKeys = QList<QByteArray>() << "Name" << "Summary" << "Version" << "Release"
                                      << "Group" << "License" << "Sources" << "Description"
                                      << "PkgConfigBR" << "PkgBR" << "Requires" << "Files";
    }

    void fetchValues(const yaml_node_t *node, QStack<QByteArray> &valueStack);
    int setValues(yaml_document_t *newDoc, const yaml_node_t *node, QList<QByteArray> &keys);
    int addValueNode(const QByteArray &key, yaml_document_t *newDoc, const yaml_node_t *nodeValue);

    QString fileName;
    YamlEditorWidget *editorWidget;

    yaml_parser_t yamlParser;
    yaml_emitter_t yamlWriter;
    yaml_document_t yamlDocument;
    bool success;

    QList<QByteArray> allKeys;
};

YamlDocument::YamlDocument(YamlEditorWidget *parent) :
    Core::IDocument(parent),
    d(new YamlDocumentPrivate)
{
    d->editorWidget = parent;
}

YamlDocument::~YamlDocument()
{
    yaml_document_delete(&d->yamlDocument);
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
            d->editorWidget->clear();
            QStack<QByteArray> valueStack;
            d->fetchValues(yaml_document_get_root_node(&d->yamlDocument), valueStack);
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
        yaml_document_t updatedDocument;
        yaml_document_initialize(&updatedDocument, d->yamlDocument.version_directive,
                                 d->yamlDocument.tag_directives.start,
                                 d->yamlDocument.tag_directives.end,
                                 d->yamlDocument.start_implicit, d->yamlDocument.end_implicit);
        QList<QByteArray> keys;
        int nodeId = d->setValues(&updatedDocument, yaml_document_get_root_node(&d->yamlDocument),
                                  keys);
        yaml_node_t *node = yaml_document_get_node(&updatedDocument, nodeId);
        if (node && node->type == YAML_MAPPING_NODE) {
            foreach (const QByteArray &k, d->allKeys) {
                if (keys.removeOne(k))
                    continue;
                const int keyNodeId = yaml_document_add_scalar(&updatedDocument, 0, (yaml_char_t *)k.data(),
                                                               k.length(), YAML_ANY_SCALAR_STYLE);
                const int valueNodeId = d->addValueNode(k, &updatedDocument, 0);
                yaml_document_append_mapping_pair(&updatedDocument, nodeId, keyNodeId, valueNodeId);
            }
        }
        d->editorWidget->setModified(false);

        FILE *yamlFile = fopen(actualName.toUtf8(), "wb");
        if (yamlFile)
            yaml_emitter_set_output_file(&d->yamlWriter, yamlFile);

        d->success = yaml_emitter_open(&d->yamlWriter);
        if (d->success)
            d->success = yaml_emitter_dump(&d->yamlWriter, &updatedDocument);
        if (d->success)
            d->success = yaml_emitter_close(&d->yamlWriter);
        if (d->success)
            d->success = yaml_emitter_flush(&d->yamlWriter);
        fclose(yamlFile);
        yaml_document_delete(&updatedDocument);
    }
    if (!d->success && errorString)
        *errorString = tr("Error %1").arg(QString::fromUtf8(d->yamlWriter.problem));
    yaml_emitter_delete(&d->yamlWriter);
    if (d->success) {
        yaml_document_delete(&d->yamlDocument);
        open(0, actualName);
    }
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

void YamlDocumentPrivate::fetchValues(const yaml_node_t *node, QStack<QByteArray> &valueStack)
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
                fetchValues(nodeItem, valueStack);
                ++current;
            }
            break;
        }
        case YAML_MAPPING_NODE: {
            yaml_node_pair_t *current = node->data.mapping.pairs.start;
            yaml_node_pair_t *end = node->data.mapping.pairs.top;
            while (current && current < end) {
                yaml_node_t *nodeKey = yaml_document_get_node(&yamlDocument, current->key);
                if (nodeKey->type != YAML_SCALAR_NODE ||
                        !allKeys.contains((char*)nodeKey->data.scalar.value)) {
                    ++current;
                    continue;
                }
                const QByteArray key = QByteArray((const char*)nodeKey->data.scalar.value);;
                yaml_node_t *nodeValue = yaml_document_get_node(&yamlDocument, current->value);
                fetchValues(nodeValue, valueStack);
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

int YamlDocumentPrivate::setValues(yaml_document_t *newDoc, const yaml_node_t *node, QList<QByteArray> &keys)
{
    int nodeId = 0;
    if (node) {
        switch (node->type) {
        case YAML_NO_NODE:
            break;
        case YAML_SCALAR_NODE:
            nodeId = yaml_document_add_scalar(newDoc, node->tag, node->data.scalar.value,
                                              node->data.scalar.length, node->data.scalar.style);
            break;
        case YAML_SEQUENCE_NODE: {
            nodeId = yaml_document_add_sequence(newDoc, node->tag, node->data.sequence.style);
            yaml_node_item_t *current = node->data.sequence.items.start;
            const yaml_node_item_t *end = node->data.sequence.items.top;
            while (current && current < end) {
                const yaml_node_t *nodeItem = yaml_document_get_node(&yamlDocument, *current);
                int itemId = setValues(newDoc, nodeItem, keys);
                yaml_document_append_sequence_item(newDoc, nodeId, itemId);
                ++current;
            }
            break;
        }
        case YAML_MAPPING_NODE: {
            nodeId = yaml_document_add_mapping(newDoc, node->tag, node->data.mapping.style);
            yaml_node_pair_t *current = node->data.mapping.pairs.start;
            const yaml_node_pair_t *end = node->data.mapping.pairs.top;
            while (current && current < end) {
                const yaml_node_t *nodeKey = yaml_document_get_node(&yamlDocument, current->key);
                if (nodeKey->type != YAML_SCALAR_NODE) {
                    ++current;
                    continue;
                }
                const int keyNodeId = yaml_document_add_scalar(newDoc, nodeKey->tag,
                                                               nodeKey->data.scalar.value,
                                                               nodeKey->data.scalar.length,
                                                               nodeKey->data.scalar.style);

                const yaml_node_t *nodeValue = yaml_document_get_node(&yamlDocument,
                                                                      current->value);
                QByteArray key = QByteArray((const char*)nodeKey->data.scalar.value);
                keys << key;
                int valueNodeId = addValueNode(key, newDoc, nodeValue);
                if (!valueNodeId) {
                    keys.removeLast();
                    valueNodeId = setValues(newDoc, nodeValue, keys);
                }
                yaml_document_append_mapping_pair(newDoc, nodeId, keyNodeId, valueNodeId);
                ++current;
            }
            break;
        }
        }
    }
    return nodeId;
}

int YamlDocumentPrivate::addValueNode(const QByteArray &key, yaml_document_t *newDoc, const yaml_node_t *nodeValue)
{
    int valueNodeId = 0;
    yaml_scalar_style_t scalarStyle = YAML_ANY_SCALAR_STYLE;
    yaml_sequence_style_t sequenceStyle = YAML_ANY_SEQUENCE_STYLE;
    if (nodeValue) {
        switch (nodeValue->type) {
        case YAML_SCALAR_NODE: scalarStyle = nodeValue->data.scalar.style;
            break;
        case YAML_SEQUENCE_NODE: sequenceStyle = nodeValue->data.sequence.style;
            break;
        default:
            break;
        }
    }
    if (key == "Name") {
        const QByteArray name = editorWidget->name().toUtf8();
        valueNodeId = yaml_document_add_scalar(newDoc, 0, (yaml_char_t *)name.data(),
                                               name.length(), scalarStyle);
    } else if (key == "Summary") {
        const QByteArray summary = editorWidget->summary().toUtf8();
        valueNodeId = yaml_document_add_scalar(newDoc, 0, (yaml_char_t *)summary.data(),
                                               summary.length(), scalarStyle);
    } else if (key == "Version") {
        const QByteArray version = editorWidget->version().toUtf8();
        valueNodeId = yaml_document_add_scalar(newDoc, 0, (yaml_char_t *)version.data(),
                                               version.length(), scalarStyle);
    } else if (key == "Release") {
        const QByteArray release = editorWidget->release().toUtf8();
        valueNodeId = yaml_document_add_scalar(newDoc, 0, (yaml_char_t *)release.data(),
                                               release.length(), scalarStyle);
    } else if (key == "Group") {
        const QByteArray group = editorWidget->group().toUtf8();
        valueNodeId = yaml_document_add_scalar(newDoc, 0, (yaml_char_t *)group.data(),
                                               group.length(), scalarStyle);
    } else if (key == "License") {
        const QByteArray license = editorWidget->license().toUtf8();
        valueNodeId = yaml_document_add_scalar(newDoc, 0, (yaml_char_t *)license.data(),
                                               license.length(), scalarStyle);
    } else if (key == "Sources") {
        valueNodeId = yaml_document_add_sequence(newDoc, 0,
                                                 sequenceStyle);
        const QStringList sources = editorWidget->sources();
        foreach (const QString &source, sources) {
            const QByteArray s = source.toUtf8();
            int id = yaml_document_add_scalar(newDoc, 0, (yaml_char_t *)s.data(),
                                              s.length(), YAML_ANY_SCALAR_STYLE);
            yaml_document_append_sequence_item(newDoc, valueNodeId, id);
        }
    } else if (key == "Description") {
        const QByteArray description = editorWidget->description().toUtf8();
        valueNodeId = yaml_document_add_scalar(newDoc, 0,
                                               (yaml_char_t *)description.data(),
                                               description.length(), scalarStyle);
    } else if (key == "PkgConfigBR") {
        valueNodeId = yaml_document_add_sequence(newDoc, 0, sequenceStyle);
        const QStringList pkgConfigs = editorWidget->pkgConfigBR();
        foreach (const QString &pc, pkgConfigs) {
            const QByteArray p = pc.toUtf8();
            int id = yaml_document_add_scalar(newDoc, 0, (yaml_char_t *)p.data(),
                                              p.length(), YAML_ANY_SCALAR_STYLE);
            yaml_document_append_sequence_item(newDoc, valueNodeId, id);
        }
    } else if (key == "PkgBR") {
        valueNodeId = yaml_document_add_sequence(newDoc, 0, sequenceStyle);
        const QStringList pkgs = editorWidget->pkgBR();
        foreach (const QString &p, pkgs) {
            const QByteArray pkg = p.toUtf8();
            int id = yaml_document_add_scalar(newDoc, 0, (yaml_char_t *)pkg.data(),
                                              pkg.length(), YAML_ANY_SCALAR_STYLE);
            yaml_document_append_sequence_item(newDoc, valueNodeId, id);
        }
    } else if (key == "Requires") {
        valueNodeId = yaml_document_add_sequence(newDoc, 0, sequenceStyle);
        const QStringList requires = editorWidget->requires();
        foreach (const QString &r, requires) {
            const QByteArray req = r.toUtf8();
            int id = yaml_document_add_scalar(newDoc, 0, (yaml_char_t *)req.data(),
                                              req.length(), YAML_ANY_SCALAR_STYLE);
            yaml_document_append_sequence_item(newDoc, valueNodeId, id);
        }
    } else if (key == "Files") {
        valueNodeId = yaml_document_add_sequence(newDoc, 0, sequenceStyle);
        const QStringList files = editorWidget->files();
        foreach (const QString &f, files) {
            const QByteArray file = f.toUtf8();
            int id = yaml_document_add_scalar(newDoc, 0, (yaml_char_t *)file.data(),
                                              file.length(), YAML_ANY_SCALAR_STYLE);
            yaml_document_append_sequence_item(newDoc, valueNodeId, id);
        }
    }
    return valueNodeId;
}

} // Internal
} // Mer
