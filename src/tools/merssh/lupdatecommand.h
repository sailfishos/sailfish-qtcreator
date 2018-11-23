/****************************************************************************
**
** Copyright (C) 2018 Jolla Ltd.
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
****************************************************************************/

#ifndef LUPDATECOMMAND_H
#define LUPDATECOMMAND_H

#include "command.h"

class LUpdateCommand : public Command
{
    Q_OBJECT

public:
    LUpdateCommand();
    QString name() const override;
    int execute() override;
    bool isValid() const override;
};

#endif // LUPDATECOMMAND_H
