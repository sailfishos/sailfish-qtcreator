/****************************************************************************
**
** Copyright (C) 2012 - 2016 Jolla Ltd.
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

#ifndef MERICONS_H
#define MERICONS_H

#include "merconstants.h"

#include <utils/icon.h>

namespace Mer {
namespace Icons {

const Utils::Icon MER_MODE_ICON_CLASSIC(
        (QLatin1String(Constants::MER_OPTIONS_CATEGORY_ICON)));
const Utils::Icon MER_MODE_ICON_FLAT({
        {QLatin1String(Constants::MER_OPTIONS_CATEGORY_ICON_MASK), Utils::Theme::IconsBaseColor}});
const Utils::Icon MER_MODE_ICON_FLAT_ACTIVE({
        {QLatin1String(Constants::MER_OPTIONS_CATEGORY_ICON_MASK), Utils::Theme::IconsModeMerActiveColor}});

} // namespace Icons
} // namespace Mer

#endif // MERICONS_H
