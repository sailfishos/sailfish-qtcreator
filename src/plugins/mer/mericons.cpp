/****************************************************************************
**
** Copyright (C) 2018-2019 Jolla Ltd.
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

#include "mericons.h"

using namespace Utils;

namespace Mer {
namespace Icons {

const Icon MER_MODE_CLASSIC(":/mer/images/icon-s-sailfish-qtcreator.png");
const Icon MER_MODE_FLAT({
        {":/mer/images/icon-s-sailfish-qtcreator-mask.png", Utils::Theme::IconsBaseColor}});
const Icon MER_MODE_FLAT_ACTIVE({
        {":/mer/images/icon-s-sailfish-qtcreator-mask.png", Utils::Theme::IconsModeMerActiveColor}});
const Icon MER_OPTIONS_CATEGORY(":/mer/images/icon-s-sailfish-qtcreator-mask-small.png");
const Icon MER_EMULATOR_RUN(":/mer/images/emulator-run.png");
const Icon MER_EMULATOR_STOP(":/mer/images/emulator-stop.png");
const Icon MER_SDK_RUN(":/mer/images/sdk-run.png");
const Icon MER_SDK_STOP(":/mer/images/sdk-stop.png");
const Icon MER_DEVICE_CLASSIC(":/mer/images/icon-s-sailfish-qtcreator.png");
const Icon MER_DEVICE_FLAT({
        {":/mer/images/sailfishdevice.png", Theme::IconsBaseColor}});
const Icon MER_DEVICE_FLAT_SMALL({
        {":/mer/images/sailfishdevicesmall.png", Theme::PanelTextColorDark}}, Icon::Tint);


} // namespace Icons
} // namespace Mer
