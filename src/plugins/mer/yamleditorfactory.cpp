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

#include "yamleditorfactory.h"
#include "yamleditorwidget.h"
#include "merconstants.h"

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>

namespace Mer {
namespace Internal {

YamlEditorFactory::YamlEditorFactory(QObject *parent) :
    Core::IEditorFactory(parent)
{
}

QStringList YamlEditorFactory::mimeTypes() const
{
    return QStringList() << QLatin1String(Constants::MER_YAML_MIME_TYPE);
}

Core::Id YamlEditorFactory::id() const
{
    return Constants::MER_YAML_EDITOR_ID;
}

QString YamlEditorFactory::displayName() const
{
    return tr("YAML editor");
}

Core::IDocument *YamlEditorFactory::open(const QString &fileName)
{
    Core::IEditor *iface = Core::EditorManager::instance()->openEditor(fileName, id());
    return iface ? iface->document() : 0;
}

Core::IEditor *YamlEditorFactory::createEditor(QWidget *parent)
{
    YamlEditorWidget *editorWidget = new YamlEditorWidget(parent);
    return editorWidget->editor();
}

} // Internal
} // Mer
