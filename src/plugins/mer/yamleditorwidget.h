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

#ifndef YAMLEDITORWIDGET_H
#define YAMLEDITORWIDGET_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QListWidgetItem;
QT_END_NAMESPACE

namespace Core {
class IEditor;
}

namespace Mer {
namespace Internal {

namespace Ui {
class YamlEditorWidget;
}

class YamlEditor;
class YamlEditorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit YamlEditorWidget(QWidget *parent = 0);
    ~YamlEditorWidget();

    Core::IEditor *editor() const;

    QString name() const;
    void setName(const QString &name);

    QString summary() const;
    void setSummary(const QString &summary);

    QString version() const;
    void setVersion(const QString &version);

    QString release() const;
    void setRelease(const QString &license);

    QString group() const;
    void setGroup(const QString &group);

    QString license() const;
    void setLicense(const QString &license);

    QStringList sources() const;
    void setSources(const QStringList &sources);

    QString description() const;
    void setDescription(const QString &description);

    QStringList pkgConfigBR() const;
    void setPkgConfigBR(const QStringList &requires);

    QStringList pkgBR() const;
    void setPkgBR(const QStringList &packages);

    QStringList requires() const;
    void setRequires(const QStringList &requires);

    QStringList files() const;
    void setFiles(const QStringList &files);

    bool modified() const;
    void setModified(bool modified);
    void setTrackChanges(bool track);

signals:
    void changed();

private slots:
    void onModified();
    void onAddFiles();
    void onRemoveFiles();
    void onAddItem();
    void onRemoveItem();
    void onItemActivated(QListWidgetItem *item);
    void onSelectionChanged();

private:
    YamlEditor *createEditor();

private:
    Ui::YamlEditorWidget *ui;
    mutable Core::IEditor *m_editor;
    bool m_modified;
    bool m_trackChanges;
};

} // Internal
} // Mer

#endif // YAMLEDITORWIDGET_H
