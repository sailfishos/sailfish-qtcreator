/****************************************************************************
**
** Copyright (C) 2012 - 2014 Jolla Ltd.
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

#ifndef MERMODE_H
#define MERMODE_H

#include <coreplugin/imode.h>

namespace Core {
class IMode;
}

namespace Mer {
namespace Internal {

class MerMode : public Core::IMode
{
    Q_OBJECT

public:
    MerMode();
private slots:
    void handleUpdateContext(Core::Id newMode, Core::Id oldMode);
};

} // namespace Internal
} // namespace Mer

#endif // MERMODE_H
