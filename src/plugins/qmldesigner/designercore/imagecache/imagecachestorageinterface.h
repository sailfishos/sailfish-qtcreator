/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include <QIcon>
#include <QImage>

#include <sqlitetimestamp.h>
#include <utils/smallstringview.h>

namespace QmlDesigner {
namespace Internal {
class ImageCacheStorageImageEntry
{
public:
    QImage image;
    bool hasEntry = false;
};

class ImageCacheStorageIconEntry
{
public:
    QIcon icon;
    bool hasEntry = false;
};

} // namespace Internal

class ImageCacheStorageInterface
{
public:
    using ImageEntry = Internal::ImageCacheStorageImageEntry;
    using IconEntry = Internal::ImageCacheStorageIconEntry;

    virtual ImageEntry fetchImage(Utils::SmallStringView name,
                                  Sqlite::TimeStamp minimumTimeStamp) const = 0;
    virtual ImageEntry fetchSmallImage(Utils::SmallStringView name,
                                       Sqlite::TimeStamp minimumTimeStamp) const = 0;
    virtual IconEntry fetchIcon(Utils::SmallStringView name,
                                Sqlite::TimeStamp minimumTimeStamp) const = 0;
    virtual void storeImage(Utils::SmallStringView name,
                            Sqlite::TimeStamp newTimeStamp,
                            const QImage &image,
                            const QImage &smallImage)
        = 0;
    virtual void storeIcon(Utils::SmallStringView name, Sqlite::TimeStamp newTimeStamp, const QIcon &icon) = 0;
    virtual void walCheckpointFull() = 0;

protected:
    ~ImageCacheStorageInterface() = default;
};

} // namespace QmlDesigner
