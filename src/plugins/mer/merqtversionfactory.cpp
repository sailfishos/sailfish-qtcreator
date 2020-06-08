/****************************************************************************
**
** Copyright (C) 2012-2015 Jolla Ltd.
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

#include "merqtversionfactory.h"

#include "merconstants.h"
#include "merqtversion.h"
#include "mersdkmanager.h"

#include <sfdk/buildengine.h>

#include <utils/qtcassert.h>

#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>

using namespace QtSupport;
using namespace Sfdk;
using namespace Utils;

namespace Mer {
namespace Internal {

MerQtVersionFactory::MerQtVersionFactory()
{
    setSupportedType(Constants::MER_QT);
    setPriority(50);
    setQtVersionCreator([]() { return new MerQtVersion; });
    setRestrictionChecker([](const SetupData &setup) {
        // sdk-manage adds that during target synchronization
        return setup.platforms.contains("sailfishos");
    });
}

MerQtVersionFactory::~MerQtVersionFactory()
{
}

} // Internal
} // Mer

