/****************************************************************************
 **
 ** Copyright (C) 2016 Jolla Ltd.
 ** Copyright (C) 2019 Open Mobile Platform LLC.
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

#include "operation.h"

class RmSfdkDeviceModelOperation: public Operation
{
public:
    RmSfdkDeviceModelOperation();
public:
    QString name() const;
    QString helpText() const;
    QString argumentsHelpText() const;

    bool setArguments(const QStringList &args);

    int execute() const;

#ifdef WITH_TESTS
    bool test() const;
#endif
    static QVariantMap removeDeviceModel(const QVariantMap &map, const QString &name);

private:
    QString m_name;
};
