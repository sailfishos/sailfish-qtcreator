/****************************************************************************
**
** Copyright (C) 2012 - 2013 Jolla Ltd.
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

#ifndef MERCONNECTIONREQUEST_H
#define MERCONNECTIONREQUEST_H

#include <QObject>
#include <QPointer>
#include <QString>

#include "merplugin.h"

namespace Mer {
namespace Internal {

class MerConnection;

class MerConnectionPrompt : public QObject
{
    Q_OBJECT
public:
    enum PromptRequest {
        Start = 0,
        Close
    };

    MerConnectionPrompt(MerConnection *connection, const Mer::Internal::MerPlugin *merplugin = 0);

public slots:
    void prompt(const MerConnectionPrompt::PromptRequest pr);

private slots:
    void startPrompt();
    void closePrompt();

private:
    QPointer<MerConnection> m_connection;
    const MerPlugin *m_merplugin;
};

}
}

#endif // MERCONNECTIONREQUEST_H
