/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <QAbstractTableModel>
#include <QDateTime>
#include <QDir>
#include <QHash>
#include <QIcon>
#include <QPair>
#include <QSet>
#include <QTimer>

QT_BEGIN_NAMESPACE
class QFileIconProvider;
class QFileSystemModel;
QT_END_NAMESPACE

namespace Utils { class FileSystemWatcher; }

namespace QmlDesigner {

class SynchronousImageCache;
class ItemLibraryFileIconProvider;

class CustomFileSystemModel : public QAbstractListModel
{
    Q_OBJECT
public:
    CustomFileSystemModel(QmlDesigner::SynchronousImageCache &fontImageCache,
                          QObject *parent = nullptr);

    void setFilter(QDir::Filters filters);
    QString rootPath() const;
    QModelIndex setRootPath(const QString &newPath);

    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex & parent = QModelIndex()) const override;
    int columnCount(const QModelIndex & parent = QModelIndex()) const override;

    QModelIndex indexForPath(const QString & path, int column = 0) const;

    QIcon fileIcon(const QModelIndex & index) const;
    QString fileName(const QModelIndex & index) const;
    QFileInfo fileInfo(const QModelIndex & index) const;

    Qt::ItemFlags flags(const QModelIndex &index) const override;
    void setSearchFilter(const QString &nameFilterList);

    QPair<QString, QByteArray> resourceTypeAndData(const QModelIndex &index) const;
    const QSet<QString> &supportedSuffixes() const;
    const QSet<QString> &previewableSuffixes() const;

private:
    QModelIndex updatePath(const QString &newPath);
    QModelIndex fileSystemModelIndex(const QModelIndex &index) const;
    void appendIfNotFiltered(const QString &file);

    QFileSystemModel *m_fileSystemModel;
    QStringList m_files;
    QString m_searchFilter;
    Utils::FileSystemWatcher *m_fileSystemWatcher;
    SynchronousImageCache &m_fontImageCache;
    ItemLibraryFileIconProvider *m_fileIconProvider = nullptr;
    QHash<QString, QPair<QDateTime, QIcon>> m_iconCache;
    QTimer m_updatePathTimer;
};

} //QmlDesigner
