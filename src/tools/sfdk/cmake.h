/****************************************************************************
**
** Copyright (C) 2021 Jolla Ltd.
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

#pragma once

#include <QtGlobal>

QT_BEGIN_NAMESPACE
class QDir;
class QJsonArray;
class QProcessEnvironment;
class QString;
class QStringList;
QT_END_NAMESPACE

namespace Sfdk {

class BuildTargetData;

class CMakeHelper
{
public:
    static void maybePrepareCMakeApiPathMapping(QProcessEnvironment *extraEnvironment);
    static void maybeDoCMakeApiPathMapping();

private:
    static bool doCMakeApiReplyPathMapping(const QDir &src, const QDir &dst, const QString &entry,
        const BuildTargetData &target);
    static bool doCMakeApiCacheReplyPathMapping(QString *cacheData, const BuildTargetData &target);
    static QString readCMakeRelativeRoot();
    static void updateOrAddToCMakeCacheIf(QJsonArray *entries, const QString &name,
            const QStringList &types, const QString &value, bool shouldAdd);
};

} // namespace Sfdk
