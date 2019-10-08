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

#include "merqtversionfactory.h"

#include "merconstants.h"
#include "merqtversion.h"
#include "mersdk.h"
#include "mersdkmanager.h"

#include <utils/qtcassert.h>

#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>

using namespace QtSupport;
using namespace Utils;

namespace Mer {
namespace Internal {

MerQtVersionFactory::MerQtVersionFactory(QObject *parent)
    : QtVersionFactory(parent)
{
}

MerQtVersionFactory::~MerQtVersionFactory()
{
}

bool MerQtVersionFactory::canRestore(const QString &type)
{
    return type == QLatin1String(Constants::MER_QT);
}

BaseQtVersion *MerQtVersionFactory::restore(const QString &type,
                                            const QVariantMap &data)
{
    QTC_ASSERT(canRestore(type), return 0);
    MerQtVersion *v = new MerQtVersion();
    v->fromMap(data);

    // Check if the qtVersion is still valid
    QFileInfo fi = v->qmakeCommand().toFileInfo();
    if (!fi.exists() || v->virtualMachineName().isEmpty() || v->targetName().isEmpty()) {
        delete v;
        return 0;
    }

    return v;
}

int MerQtVersionFactory::priority() const
{
    return 50;
}

BaseQtVersion *MerQtVersionFactory::create(const FileName &qmakeCommand,
                                           ProFileEvaluator * /*evaluator*/,
                                           bool isAutoDetected,
                                           const QString &autoDetectionSource)
{
    const QFileInfo fi = qmakeCommand.toFileInfo();
    if (!fi.exists() || !fi.isExecutable() || !fi.isFile())
        return 0;

    // Check for the location of qmake
    bool found = qmakeCommand.toString().contains(MerSdkManager::globalSdkToolsDirectory());
    if (!found)
        found = qmakeCommand.toString().contains(MerSdkManager::sdkToolsDirectory());
    if (!found)
        return 0;
    return new MerQtVersion(qmakeCommand, isAutoDetected, autoDetectionSource);
}

} // Internal
} // Mer

