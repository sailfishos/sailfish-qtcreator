/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <QLinkedList>
#include <QTime>
#include <QWidget>

#include <languageserverprotocol/basemessage.h>

namespace LanguageClient {

struct LspLogMessage
{
    enum MessageSender { ClientMessage, ServerMessage } sender;
    QTime time;
    LanguageServerProtocol::BaseMessage message;
};

class LspLogger : public QObject
{
    Q_OBJECT
public:
    LspLogger() {}

    QWidget *createWidget();


    void log(const LspLogMessage::MessageSender sender,
             const QString &clientName,
             const LanguageServerProtocol::BaseMessage &message);

    QLinkedList<LspLogMessage> messages(const QString &clientName) const;
    QList<QString> clients() const;

signals:
    void newMessage(const QString &clientName, const LspLogMessage &message);

private:
    QMap<QString, QLinkedList<LspLogMessage>> m_logs;
    int m_logSize = 100; // default log size if no widget is currently visible
};

} // namespace LanguageClient
