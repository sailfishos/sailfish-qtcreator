/****************************************************************************
**
** Copyright (C) 2012-2015 Jolla Ltd.
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

#ifndef MERQTVERSIONFACTORY_H
#define MERQTVERSIONFACTORY_H

#include <qtsupport/qtversionfactory.h>

namespace Mer {
namespace Internal {

class MerQtVersionFactory : public QtSupport::QtVersionFactory
{
public:
    explicit MerQtVersionFactory(QObject *parent = 0);
    ~MerQtVersionFactory() override;

    bool canRestore(const QString &type) override;
    QtSupport::BaseQtVersion *restore(const QString &type, const QVariantMap &data) override;

    int priority() const override;
    QtSupport::BaseQtVersion *create(const Utils::FileName &qmakePath,
                                     ProFileEvaluator *evaluator,
                                     bool isAutoDetected,
                                     const QString &autoDetectionSource) override;
};

} // Internal
} // Mer

#endif // MERQTVERSIONFACTORY_H
