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

#include "itemlibraryinfo.h"
#include "nodemetainfo.h"

#include <QSharedData>

#include <utils/algorithm.h>
#include <utils/fileutils.h>

namespace QmlDesigner {

namespace Internal {

class ItemLibraryEntryData
{
public:
    QString name;
    TypeName typeName;
    QString category;
    int majorVersion{-1};
    int minorVersion{-1};
    QString libraryEntryIconPath;
    QIcon typeIcon = QIcon(":/ItemLibrary/images/item-default-icon.png");
    QList<PropertyContainer> properties;
    QString qml;
    QString qmlSource;
    QString requiredImport;
    QHash<QString, QString> hints;
    QString customComponentSource;
    QStringList extraFilePaths;
};

} // namespace Internal

void ItemLibraryEntry::setTypeIcon(const QIcon &icon)
{
    m_data->typeIcon = icon;
}

void ItemLibraryEntry::addProperty(const Property &property)
{
    m_data->properties.append(property);
}

QList<ItemLibraryEntry::Property> ItemLibraryEntry::properties() const
{
    return m_data->properties;
}

QHash<QString, QString> ItemLibraryEntry::hints() const
{
    return m_data->hints;
}

ItemLibraryEntry::ItemLibraryEntry() : m_data(new Internal::ItemLibraryEntryData)
{
    m_data->name.clear();
}

QString ItemLibraryEntry::name() const
{
    return m_data->name;
}

TypeName ItemLibraryEntry::typeName() const
{
    return m_data->typeName;
}

QString ItemLibraryEntry::qmlPath() const
{
    return m_data->qml;
}

QString ItemLibraryEntry::qmlSource() const
{
    return m_data->qmlSource;
}

QString ItemLibraryEntry::requiredImport() const
{
    return m_data->requiredImport;
}

QString ItemLibraryEntry::customComponentSource() const
{
    return m_data->customComponentSource;
}

QStringList ItemLibraryEntry::extraFilePaths() const
{
    return m_data->extraFilePaths;
}

int ItemLibraryEntry::majorVersion() const
{
    return m_data->majorVersion;
}

int ItemLibraryEntry::minorVersion() const
{
    return m_data->minorVersion;
}

QString ItemLibraryEntry::category() const
{
    return m_data->category;
}

void ItemLibraryEntry::setCategory(const QString &category)
{
    m_data->category = category;
}

QIcon ItemLibraryEntry::typeIcon() const
{
    return m_data->typeIcon;
}

QString ItemLibraryEntry::libraryEntryIconPath() const
{
    return m_data->libraryEntryIconPath;
}

void ItemLibraryEntry::setName(const QString &name)
{
     m_data->name = name;
}

void ItemLibraryEntry::setType(const TypeName &typeName, int majorVersion, int minorVersion)
{
    m_data->typeName = typeName;
    m_data->majorVersion = majorVersion;
    m_data->minorVersion = minorVersion;
}

void ItemLibraryEntry::setLibraryEntryIconPath(const QString &iconPath)
{
    m_data->libraryEntryIconPath = iconPath;
}

static QByteArray getSourceForUrl(const QString &fileURl)
{
    Utils::FileReader fileReader;

    if (fileReader.fetch(fileURl))
        return fileReader.data();
    else
        return Utils::FileReader::fetchQrc(fileURl);
}

void ItemLibraryEntry::setQmlPath(const QString &qml)
{
    m_data->qml = qml;

    m_data->qmlSource = QString::fromUtf8(getSourceForUrl(qml));
}

void ItemLibraryEntry::setRequiredImport(const QString &requiredImport)
{
    m_data->requiredImport = requiredImport;
}

void ItemLibraryEntry::addHints(const QHash<QString, QString> &hints)
{
    Utils::addToHash(&m_data->hints, hints);
}

void ItemLibraryEntry::setCustomComponentSource(const QString &source)
{
    m_data->customComponentSource = source;
}

void ItemLibraryEntry::addExtraFilePath(const QString &extraFile)
{
    m_data->extraFilePaths.append(extraFile);
}

void ItemLibraryEntry::addProperty(PropertyName &name, QString &type, QVariant &value)
{
    Property property;
    property.set(name, type, value);
    addProperty(property);
}

QDataStream& operator<<(QDataStream& stream, const ItemLibraryEntry &itemLibraryEntry)
{
    stream << itemLibraryEntry.name();
    stream << itemLibraryEntry.typeName();
    stream << itemLibraryEntry.majorVersion();
    stream << itemLibraryEntry.minorVersion();
    stream << itemLibraryEntry.typeIcon();
    stream << itemLibraryEntry.libraryEntryIconPath();
    stream << itemLibraryEntry.category();
    stream << itemLibraryEntry.requiredImport();
    stream << itemLibraryEntry.hints();

    stream << itemLibraryEntry.m_data->properties;
    stream << itemLibraryEntry.m_data->qml;
    stream << itemLibraryEntry.m_data->qmlSource;
    stream << itemLibraryEntry.m_data->customComponentSource;
    stream << itemLibraryEntry.m_data->extraFilePaths;

    return stream;
}

QDataStream& operator>>(QDataStream& stream, ItemLibraryEntry &itemLibraryEntry)
{
    // Clear containers so that we don't simply append to them in case the object is reused
    itemLibraryEntry.m_data->hints.clear();
    itemLibraryEntry.m_data->properties.clear();

    stream >> itemLibraryEntry.m_data->name;
    stream >> itemLibraryEntry.m_data->typeName;
    stream >> itemLibraryEntry.m_data->majorVersion;
    stream >> itemLibraryEntry.m_data->minorVersion;
    stream >> itemLibraryEntry.m_data->typeIcon;
    stream >> itemLibraryEntry.m_data->libraryEntryIconPath;
    stream >> itemLibraryEntry.m_data->category;
    stream >> itemLibraryEntry.m_data->requiredImport;
    stream >> itemLibraryEntry.m_data->hints;

    stream >> itemLibraryEntry.m_data->properties;
    stream >> itemLibraryEntry.m_data->qml;
    stream >> itemLibraryEntry.m_data->qmlSource;
    stream >> itemLibraryEntry.m_data->customComponentSource;
    stream >> itemLibraryEntry.m_data->extraFilePaths;

    return stream;
}

QDebug operator<<(QDebug debug, const ItemLibraryEntry &itemLibraryEntry)
{
    debug << itemLibraryEntry.m_data->name;
    debug << itemLibraryEntry.m_data->typeName;
    debug << itemLibraryEntry.m_data->majorVersion;
    debug << itemLibraryEntry.m_data->minorVersion;
    debug << itemLibraryEntry.m_data->typeIcon;
    debug << itemLibraryEntry.m_data->libraryEntryIconPath;
    debug << itemLibraryEntry.m_data->category;
    debug << itemLibraryEntry.m_data->requiredImport;
    debug << itemLibraryEntry.m_data->hints;

    debug << itemLibraryEntry.m_data->properties;
    debug << itemLibraryEntry.m_data->qml;
    debug << itemLibraryEntry.m_data->qmlSource;
    debug << itemLibraryEntry.m_data->customComponentSource;
    debug << itemLibraryEntry.m_data->extraFilePaths;

    return debug.space();
}

//
// ItemLibraryInfo
//

ItemLibraryInfo::ItemLibraryInfo(QObject *parent)
    : QObject(parent)
{
}



QList<ItemLibraryEntry> ItemLibraryInfo::entriesForType(const QByteArray &typeName, int majorVersion, int minorVersion) const
{
    QList<ItemLibraryEntry> entries;

    foreach (const ItemLibraryEntry &entry, m_nameToEntryHash) {
        if (entry.typeName() == typeName)
            entries += entry;
    }

    if (m_baseInfo)
        entries += m_baseInfo->entriesForType(typeName, majorVersion, minorVersion);

    return entries;
}

QList<ItemLibraryEntry> ItemLibraryInfo::entries() const
{
    QList<ItemLibraryEntry> list = m_nameToEntryHash.values();
    if (m_baseInfo)
        list += m_baseInfo->entries();
    return list;
}

static inline QString keyForEntry(const ItemLibraryEntry &entry)
{
    return entry.name() + entry.category() + QString::number(entry.majorVersion());
}

void ItemLibraryInfo::addEntries(const QList<ItemLibraryEntry> &entries, bool overwriteDuplicate)
{
    foreach (const ItemLibraryEntry &entry, entries) {
        const QString key = keyForEntry(entry);
        if (!overwriteDuplicate && m_nameToEntryHash.contains(key))
            throw InvalidMetaInfoException(__LINE__, __FUNCTION__, __FILE__);
        m_nameToEntryHash.insert(key, entry);
    }
    emit entriesChanged();
}

bool ItemLibraryInfo::containsEntry(const ItemLibraryEntry &entry)
{
    const QString key = keyForEntry(entry);
    return m_nameToEntryHash.contains(key);
}

void ItemLibraryInfo::clearEntries()
{
    m_nameToEntryHash.clear();
    emit entriesChanged();
}

QStringList ItemLibraryInfo::blacklistImports() const
{
    auto list = m_blacklistImports;
    if (m_baseInfo)
        list.append(m_baseInfo->m_blacklistImports);
    return list;
}

QStringList ItemLibraryInfo::showTagsForImports() const
{
    auto list = m_showTagsForImports;
    if (m_baseInfo)
        list.append(m_baseInfo->m_showTagsForImports);
    list.removeDuplicates();
    return list;
}

void ItemLibraryInfo::addBlacklistImports(const QStringList &list)
{
    m_blacklistImports.append(list);
}

void ItemLibraryInfo::addShowTagsForImports(const QStringList &list)
{
    if (!list.isEmpty()) {
        m_showTagsForImports.append(list);
        emit importTagsChanged();
    }
}

void ItemLibraryInfo::setBaseInfo(ItemLibraryInfo *baseInfo)
{
    m_baseInfo = baseInfo;
}

} // namespace QmlDesigner
