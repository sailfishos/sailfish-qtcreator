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

#ifndef YAMLDOCUMENT_H
#define YAMLDOCUMENT_H

#include <coreplugin/textdocument.h>

namespace Mer {
namespace Internal {

class YamlEditorWidget;
class YamlDocument : public Core::TextDocument
{
    Q_OBJECT
public:
    explicit YamlDocument(YamlEditorWidget *parent);

    bool open(QString *errorString, const QString &fileName);
    bool save(QString *errorString, const QString &fileName = QString(), bool autoSave = false);
    QString fileName() const;

    QString defaultPath() const;
    QString suggestedFileName() const;
    QString mimeType() const;

    bool isModified() const;
    bool isSaveAsAllowed() const;

    bool reload(QString *errorString, ReloadFlag flag, ChangeType type);
    void rename(const QString &newName);
    bool loadContent(const QString &yamlSource, QString *errorMessage = 0, int *errorLine = 0);

private:
    QString m_fileName;
    YamlEditorWidget *m_editorWidget;
};

} // Internal
} // Mer

#endif // YAMLDOCUMENT_H
