/****************************************************************************
**
** Copyright (C) 2012-2015,2018-2019 Jolla Ltd.
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

#include "sfdkglobal.h"

#include <QMetaType>
#include <QObject>

#include <memory>

namespace Utils { class FileName; }

namespace Sfdk {

class BuildTargetDump;

class TargetsXmlReaderPrivate;
class TargetsXmlReader : public QObject
{
public:
    explicit TargetsXmlReader(const QString &fileName, QObject *parent = nullptr);
    ~TargetsXmlReader() override;

    bool hasError() const;
    QString errorString() const;

    QList<BuildTargetDump> targets() const;
    int version() const;

private:
    std::unique_ptr<TargetsXmlReaderPrivate> d_ptr;
    Q_DECLARE_PRIVATE(TargetsXmlReader)
    Q_DISABLE_COPY(TargetsXmlReader)
};

} // namespace Sfdk
