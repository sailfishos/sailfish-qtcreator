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

#include <QByteArray>
#include <QList>

#include <functional>

namespace ProjectExplorer { class Kit; }

namespace CMakeProjectManager {

class CMakeConfigItem {
public:
    enum Type { FILEPATH, PATH, BOOL, STRING, INTERNAL, STATIC };
    CMakeConfigItem();
    CMakeConfigItem(const CMakeConfigItem &other);
    CMakeConfigItem(const QByteArray &k, Type t, const QByteArray &d, const QByteArray &v);
    CMakeConfigItem(const QByteArray &k, const QByteArray &v);

    static QByteArray valueOf(const QByteArray &key, const QList<CMakeConfigItem> &input);
    static QString expandedValueOf(const ProjectExplorer::Kit *k, const QByteArray &key,
                                   const QList<CMakeConfigItem> &input);
    static QStringList cmakeSplitValue(const QString &in, bool keepEmpty = false);
    bool isNull() const { return key.isEmpty(); }

    QString expandedValue(const ProjectExplorer::Kit *k) const;

    static std::function<bool(const CMakeConfigItem &a, const CMakeConfigItem &b)> sortOperator();
    static CMakeConfigItem fromString(const QString &s);
    QString toString() const;

    bool operator==(const CMakeConfigItem &o) const;

    QByteArray key;
    Type type = STRING;
    bool isAdvanced = false;
    QByteArray value; // converted to string as needed
    QByteArray documentation;
    QStringList values;
};
using CMakeConfig = QList<CMakeConfigItem>;

} // namespace CMakeProjectManager
