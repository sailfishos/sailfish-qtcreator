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

#include "operation.h"

#include <QString>

class AddToolChainOperation : public Operation
{
public:
    QString name() const;
    QString helpText() const;
    QString argumentsHelpText() const;

    bool setArguments(const QStringList &args);

    int execute() const;

#ifdef WITH_TESTS
    bool test() const;
#endif

    static QVariantMap addToolChain(const QVariantMap &map,
                                    const QString &id, const QString &displayName,
                                    const QString &path, const QString &abi,
                                    const QString &supportedAbis,
                                    const KeyValuePairList &extra);

    static QVariantMap initializeToolChains();
    static bool exists(const QString &id);
    static bool exists(const QVariantMap &map, const QString &id);

private:
    QString m_id;
    QString m_displayName;
    QString m_path;
    QString m_targetAbi;
    QString m_supportedAbis;
    KeyValuePairList m_extra;
};
