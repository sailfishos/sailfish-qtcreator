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

#ifndef YAMLEDITOR_H
#define YAMLEDITOR_H

#include <coreplugin/editormanager/ieditor.h>

#include <qglobal.h>

QT_BEGIN_NAMESPACE
class QToolBar;
QT_END_NAMESPACE

namespace Mer {
namespace Internal {

class YamlDocument;
class YamlEditorWidget;
class YamlEditor : public Core::IEditor
{
    Q_OBJECT
public:
    explicit YamlEditor(YamlEditorWidget *parent);

    bool createNew(const QString &contents = QString());
    bool open(QString *errorString, const QString &fileName, const QString &realFileName);
    Core::IDocument *document();
    Core::Id id() const;
    QString displayName() const;
    void setDisplayName(const QString &title);

    bool duplicateSupported() const;
    IEditor *duplicate(QWidget *parent);

    QByteArray saveState() const;
    bool restoreState(const QByteArray &state);

    bool isTemporary() const;

    QWidget *toolBar();

private:
    QString m_displayName;
    QToolBar *m_toolBar;
    YamlDocument *m_file;
};

} // Internal
} // Mer

#endif // YAMLEDITOR_H
