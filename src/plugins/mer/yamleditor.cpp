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

#include "yamleditor.h"
#include "yamleditorwidget.h"
#include "merconstants.h"
#include "yamldocument.h"

#include <QToolBar>

#include <utils/qtcassert.h>

namespace Mer {
namespace Internal {

YamlEditor::YamlEditor(YamlEditorWidget *parent) :
    Core::IEditor(parent),
    m_toolBar(new QToolBar(parent)),
    m_file(new YamlDocument(parent))
{
    setWidget(parent);
}

bool YamlEditor::createNew(const QString &/*contents*/)
{
    return false;
}

bool YamlEditor::open(QString *errorString, const QString &fileName, const QString &realFileName)
{
    QTC_ASSERT(fileName == realFileName, return false);
    return m_file->open(errorString, fileName);
}

Core::IDocument *YamlEditor::document()
{
    return m_file;
}

Core::Id YamlEditor::id() const
{
    return Constants::MER_YAML_EDITOR_ID;
}

QString YamlEditor::displayName() const
{
return m_displayName;
}

void YamlEditor::setDisplayName(const QString &title)
{
    if (title == m_displayName)
        return;

    m_displayName = title;
    emit changed();
}

bool YamlEditor::duplicateSupported() const
{
    return false;
}

Core::IEditor *YamlEditor::duplicate(QWidget *parent)
{
    Q_UNUSED(parent);
    return 0;
}

QByteArray YamlEditor::saveState() const
{
    return QByteArray(); // Not supported
}

bool YamlEditor::restoreState(const QByteArray &state)
{
    Q_UNUSED(state);
    return false; // Not supported
}

bool YamlEditor::isTemporary() const
{
    return false;
}

QWidget *YamlEditor::toolBar()
{
    return m_toolBar;
}

} // Internal
} // Mer
