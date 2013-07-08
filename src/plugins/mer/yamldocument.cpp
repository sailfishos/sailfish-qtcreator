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

class YAMLNodes {
public:
    YAMLNodes()
    {
        allKeys = QList<QByteArray>() << "Name" << "Summary" << "Version" << "Release"
                                      << "Group" << "License" << "Sources" << "Description"
                                      << "PkgConfigBR" << "PkgBR" << "Requires" << "Files";
    }

    QString name;
    QString summary;
    QString version;
    QString release;
    QString group;
    QString license;
    QString description;
    QStringList sources;
    QStringList pkgConfigBR;
    QStringList pkgBR;
    QStringList requires;
    QStringList files;

    QList<QByteArray> allKeys;
};

class YamlDocumentPrivate
{
public:
    YamlDocumentPrivate() : success(false) {}

    static void fetchValues(yaml_document_t *yamlDoc, const yaml_node_t *node, YAMLNodes &nodes, QStack<QByteArray> &valueStack);
    static int setValues(yaml_document_t *yamlDoc, yaml_document_t *newDoc, const yaml_node_t *node, const YAMLNodes &nodes, QList<QByteArray> &keys);
    static int addValueNode( const QByteArray &key, yaml_document_t *newDoc, const yaml_node_t *nodeValue = 0, const YAMLNodes &nodes = YAMLNodes());

    QString fileName;
    YamlEditorWidget *editorWidget;

    yaml_parser_t yamlParser;
    yaml_emitter_t yamlWriter;
    yaml_document_t yamlDocument;
    bool success;
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
        if (!yamlFile)
            return false;
        yaml_parser_set_input_file(&d->yamlParser, yamlFile);
        d->success = yaml_parser_load(&d->yamlParser, &d->yamlDocument);

        d->fileName = fileName;
        d->editorWidget->editor()->setDisplayName(QFileInfo(fileName).fileName());

        if (d->success) {
            d->editorWidget->setTrackChanges(false);
            d->editorWidget->clear();
            QStack<QByteArray> valueStack;
            YAMLNodes nodes;
            d->fetchValues(&d->yamlDocument, yaml_document_get_root_node(&d->yamlDocument), nodes, valueStack);

            d->editorWidget->setName(nodes.name);
            d->editorWidget->setSummary(nodes.summary);
            d->editorWidget->setVersion(nodes.version);
            d->editorWidget->setRelease(nodes.release);
            d->editorWidget->setGroup(nodes.group);
            d->editorWidget->setLicense(nodes.license);
            d->editorWidget->setSources(nodes.sources);
            d->editorWidget->setDescription(nodes.description);
            d->editorWidget->setPkgConfigBR(nodes.pkgConfigBR);
            d->editorWidget->setPkgBR(nodes.pkgBR);
            d->editorWidget->setRequires(nodes.requires);
            d->editorWidget->setFiles(nodes.files);

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
        FILE *yamlFile = fopen(actualName.toUtf8(), "wb");
        if (!yamlFile)
            return false;
        yaml_emitter_set_output_file(&d->yamlWriter, yamlFile);

        yaml_document_t updatedDocument;
        yaml_document_initialize(&updatedDocument, d->yamlDocument.version_directive,
                                 d->yamlDocument.tag_directives.start,
                                 d->yamlDocument.tag_directives.end,
                                 d->yamlDocument.start_implicit, d->yamlDocument.end_implicit);
        QList<QByteArray> keys;
        YAMLNodes nodes;
        nodes.name = d->editorWidget->name();
        nodes.summary = d->editorWidget->summary();
        nodes.version = d->editorWidget->version();
        nodes.release = d->editorWidget->release();
        nodes.group = d->editorWidget->group();
        nodes.license = d->editorWidget->license();
        nodes.sources = d->editorWidget->sources();
        nodes.description = d->editorWidget->description();
        nodes.pkgConfigBR = d->editorWidget->pkgConfigBR();
        nodes.pkgBR = d->editorWidget->pkgBR();
        nodes.requires = d->editorWidget->requires();
        nodes.files = d->editorWidget->files();

        int nodeId = d->setValues(&d->yamlDocument, &updatedDocument, yaml_document_get_root_node(&d->yamlDocument),
                                  nodes, keys);
        yaml_node_t *node = yaml_document_get_node(&updatedDocument, nodeId);
        if (node && node->type == YAML_MAPPING_NODE) {
            foreach (const QByteArray &k, nodes.allKeys) {
                if (keys.removeOne(k))
                    continue;
                const int keyNodeId = yaml_document_add_scalar(&updatedDocument, 0, (yaml_char_t *)k.data(),
                                                               k.length(), YAML_ANY_SCALAR_STYLE);
                const int valueNodeId = d->addValueNode(k, &updatedDocument);
                yaml_document_append_mapping_pair(&updatedDocument, nodeId, keyNodeId, valueNodeId);
            }
        }
        d->editorWidget->setModified(false);

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

bool YamlDocument::updateFiles(const QStringList &files, const QString &fileName)
{
    yaml_parser_t yamlParser;
    yaml_document_t yamlDocument;

    YAMLNodes nodes;
    bool success = yaml_parser_initialize(&yamlParser);
    if (success) {
        FILE *yamlFile = fopen(fileName.toUtf8(), "rb");
        if (!yamlFile)
            return false;
        yaml_parser_set_input_file(&yamlParser, yamlFile);
        success = yaml_parser_load(&yamlParser, &yamlDocument);
        if (success) {
            QStack<QByteArray> valueStack;
            YamlDocumentPrivate::fetchValues(&yamlDocument, yaml_document_get_root_node(&yamlDocument), nodes, valueStack);
        }
        fclose(yamlFile);
    }

    if (!success)
        return success;

    yaml_emitter_t yamlWriter;
    success = yaml_emitter_initialize(&yamlWriter);
    if (success) {
        yaml_document_t updatedDocument;
        yaml_document_initialize(&updatedDocument, yamlDocument.version_directive,
                                 yamlDocument.tag_directives.start,
                                 yamlDocument.tag_directives.end,
                                 yamlDocument.start_implicit, yamlDocument.end_implicit);
        foreach (const QString &file, files) {
            if (!nodes.files.contains(file))
                nodes.files.append(file);
        }
        QList<QByteArray> keys;
        YamlDocumentPrivate::setValues(&yamlDocument, &updatedDocument, yaml_document_get_root_node(&yamlDocument), nodes, keys);

        FILE *yamlFile = fopen(fileName.toUtf8(), "wb");
        if (yamlFile)
            yaml_emitter_set_output_file(&yamlWriter, yamlFile);

        success = yaml_emitter_open(&yamlWriter);
        if (success)
            success = yaml_emitter_dump(&yamlWriter, &updatedDocument);
        if (success)
            success = yaml_emitter_close(&yamlWriter);
        if (success)
            success = yaml_emitter_flush(&yamlWriter);
        fclose(yamlFile);
        yaml_document_delete(&updatedDocument);
    }

    yaml_emitter_delete(&yamlWriter);
    yaml_parser_delete(&yamlParser);
    yaml_document_delete(&yamlDocument);
    return success;
}

void YamlDocumentPrivate::fetchValues(yaml_document_t *yamlDoc, const yaml_node_t *node, YAMLNodes &nodes,
                                      QStack<QByteArray> &valueStack)
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
                yaml_node_t *nodeItem = yaml_document_get_node(yamlDoc, *current);
                fetchValues(yamlDoc, nodeItem, nodes, valueStack);
                ++current;
            }
            break;
        }
        case YAML_MAPPING_NODE: {
            yaml_node_pair_t *current = node->data.mapping.pairs.start;
            yaml_node_pair_t *end = node->data.mapping.pairs.top;
            while (current && current < end) {
                yaml_node_t *nodeKey = yaml_document_get_node(yamlDoc, current->key);
                if (nodeKey->type != YAML_SCALAR_NODE ||
                        !nodes.allKeys.contains((char*)nodeKey->data.scalar.value)) {
                    ++current;
                    continue;
                }
                const QByteArray key = QByteArray((const char*)nodeKey->data.scalar.value);;
                yaml_node_t *nodeValue = yaml_document_get_node(yamlDoc, current->value);
                fetchValues(yamlDoc, nodeValue, nodes, valueStack);
                if (key == "Name") {
                    nodes.name = QString::fromUtf8(valueStack.pop());
                } else if (key == "Summary") {
                    nodes.summary = QString::fromUtf8(valueStack.pop());
                } else if (key == "Version") {
                    nodes.version = QString::fromUtf8(valueStack.pop());
                } else if (key == "Release") {
                    nodes.release = QString::fromUtf8(valueStack.pop());
                } else if (key == "Group") {
                    nodes.group = QString::fromUtf8(valueStack.pop());
                } else if (key == "License") {
                    nodes.license = QString::fromUtf8(valueStack.pop());
                } else if (key == "Sources") {
                    QStringList sources;
                    while (!valueStack.isEmpty())
                        sources << QString::fromUtf8(valueStack.pop());
                    nodes.sources = sources;
                } else if (key == "Description") {
                    nodes.description = QString::fromUtf8(valueStack.pop());
                } else if (key == "PkgConfigBR") {
                    QStringList pkgConfigs;
                    while (!valueStack.isEmpty())
                        pkgConfigs << QString::fromUtf8(valueStack.pop());
                    nodes.pkgConfigBR = pkgConfigs;
                } else if (key == "PkgBR") {
                    QStringList pkgs;
                    while (!valueStack.isEmpty())
                        pkgs << QString::fromUtf8(valueStack.pop());
                    nodes.pkgBR = pkgs;
                } else if (key == "Requires") {
                    QStringList requires;
                    while (!valueStack.isEmpty())
                        requires << QString::fromUtf8(valueStack.pop());
                    nodes.requires = requires;
                } else if (key == "Files") {
                    QStringList files;
                    while (!valueStack.isEmpty())
                        files << QString::fromUtf8(valueStack.pop());
                    nodes.files = files;
                }
                ++current;
            }
            break;
        }
        }
    }
}

int YamlDocumentPrivate::setValues(yaml_document_t *yamlDoc, yaml_document_t *newDoc, const yaml_node_t *node, const YAMLNodes &nodes, QList<QByteArray> &keys)
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
                const yaml_node_t *nodeItem = yaml_document_get_node(yamlDoc, *current);
                int itemId = setValues(yamlDoc, newDoc, nodeItem, nodes, keys);
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
                const yaml_node_t *nodeKey = yaml_document_get_node(yamlDoc, current->key);
                if (nodeKey->type != YAML_SCALAR_NODE) {
                    ++current;
                    continue;
                }
                const int keyNodeId = yaml_document_add_scalar(newDoc, nodeKey->tag,
                                                               nodeKey->data.scalar.value,
                                                               nodeKey->data.scalar.length,
                                                               nodeKey->data.scalar.style);

                const yaml_node_t *nodeValue = yaml_document_get_node(yamlDoc, current->value);
                QByteArray key = QByteArray((const char*)nodeKey->data.scalar.value);
                keys << key;
                int valueNodeId = addValueNode(key, newDoc, nodeValue, nodes);
                if (!valueNodeId) {
                    keys.removeLast();
                    valueNodeId = setValues(yamlDoc, newDoc, nodeValue, nodes, keys);
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

int YamlDocumentPrivate::addValueNode(const QByteArray &key, yaml_document_t *newDoc, const yaml_node_t *nodeValue, const YAMLNodes &nodes)
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
        const QByteArray name = nodes.name.toUtf8();
        valueNodeId = yaml_document_add_scalar(newDoc, 0, (yaml_char_t *)name.data(),
                                               name.length(), scalarStyle);
    } else if (key == "Summary") {
        const QByteArray summary = nodes.summary.toUtf8();
        valueNodeId = yaml_document_add_scalar(newDoc, 0, (yaml_char_t *)summary.data(),
                                               summary.length(), scalarStyle);
    } else if (key == "Version") {
        const QByteArray version = nodes.version.toUtf8();
        valueNodeId = yaml_document_add_scalar(newDoc, 0, (yaml_char_t *)version.data(),
                                               version.length(), scalarStyle);
    } else if (key == "Release") {
        const QByteArray release =nodes.release.toUtf8();
        valueNodeId = yaml_document_add_scalar(newDoc, 0, (yaml_char_t *)release.data(),
                                               release.length(), scalarStyle);
    } else if (key == "Group") {
        const QByteArray group = nodes.group.toUtf8();
        valueNodeId = yaml_document_add_scalar(newDoc, 0, (yaml_char_t *)group.data(),
                                               group.length(), scalarStyle);
    } else if (key == "License") {
        const QByteArray license = nodes.license.toUtf8();
        valueNodeId = yaml_document_add_scalar(newDoc, 0, (yaml_char_t *)license.data(),
                                               license.length(), scalarStyle);
    } else if (key == "Sources") {
        valueNodeId = yaml_document_add_sequence(newDoc, 0,
                                                 sequenceStyle);
        const QStringList sources = nodes.sources;
        foreach (const QString &source, sources) {
            const QByteArray s = source.toUtf8();
            int id = yaml_document_add_scalar(newDoc, 0, (yaml_char_t *)s.data(),
                                              s.length(), YAML_ANY_SCALAR_STYLE);
            yaml_document_append_sequence_item(newDoc, valueNodeId, id);
        }
    } else if (key == "Description") {
        const QByteArray description = nodes.description.toUtf8();
        valueNodeId = yaml_document_add_scalar(newDoc, 0,
                                               (yaml_char_t *)description.data(),
                                               description.length(), scalarStyle);
    } else if (key == "PkgConfigBR") {
        valueNodeId = yaml_document_add_sequence(newDoc, 0, sequenceStyle);
        const QStringList pkgConfigs = nodes.pkgConfigBR;
        foreach (const QString &pc, pkgConfigs) {
            const QByteArray p = pc.toUtf8();
            int id = yaml_document_add_scalar(newDoc, 0, (yaml_char_t *)p.data(),
                                              p.length(), YAML_ANY_SCALAR_STYLE);
            yaml_document_append_sequence_item(newDoc, valueNodeId, id);
        }
    } else if (key == "PkgBR") {
        valueNodeId = yaml_document_add_sequence(newDoc, 0, sequenceStyle);
        const QStringList pkgs = nodes.pkgBR;
        foreach (const QString &p, pkgs) {
            const QByteArray pkg = p.toUtf8();
            int id = yaml_document_add_scalar(newDoc, 0, (yaml_char_t *)pkg.data(),
                                              pkg.length(), YAML_ANY_SCALAR_STYLE);
            yaml_document_append_sequence_item(newDoc, valueNodeId, id);
        }
    } else if (key == "Requires") {
        valueNodeId = yaml_document_add_sequence(newDoc, 0, sequenceStyle);
        const QStringList requires = nodes.requires;
        foreach (const QString &r, requires) {
            const QByteArray req = r.toUtf8();
            int id = yaml_document_add_scalar(newDoc, 0, (yaml_char_t *)req.data(),
                                              req.length(), YAML_ANY_SCALAR_STYLE);
            yaml_document_append_sequence_item(newDoc, valueNodeId, id);
        }
    } else if (key == "Files") {
        valueNodeId = yaml_document_add_sequence(newDoc, 0, sequenceStyle);
        const QStringList files = nodes.files;
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
