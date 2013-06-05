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

#include <QFileInfo>
#include <QDir>

namespace Mer {
namespace Internal {

YamlDocument::YamlDocument(YamlEditorWidget *parent) :
    Core::TextDocument(parent),
    m_editorWidget(parent)
{
}

bool YamlDocument::open(QString *errorString, const QString &fileName)
{
    QString contents;
    if (read(fileName, &contents, errorString) != Utils::TextFileFormat::ReadSuccess)
        return false;

    m_fileName = fileName;
    m_editorWidget->editor()->setDisplayName(QFileInfo(fileName).fileName());

    bool result = loadContent(contents);

    if (!result)
        *errorString = tr("%1 does not appear to be a valid application descriptor file").arg(QDir::toNativeSeparators(fileName));

    return result;
}

bool YamlDocument::save(QString *errorString, const QString &fileName, bool autoSave)
{
    return false;
}

QString YamlDocument::fileName() const
{
    return m_fileName;
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
    return false;
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

    return open(errorString, m_fileName);
}

void YamlDocument::rename(const QString &newName)
{
    const QString oldFilename = m_fileName;
    m_fileName = newName;
    m_editorWidget->editor()->setDisplayName(QFileInfo(m_fileName).fileName());
    emit fileNameChanged(oldFilename, newName);
    emit changed();
}

bool YamlDocument::loadContent(const QString &yamlSource, QString *errorMessage, int *errorLine)
{
    return true;
}

} // Internal
} // Mer
