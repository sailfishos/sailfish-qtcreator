/****************************************************************************
**
** Copyright (C) 2012-2015,2018 Jolla Ltd.
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

#ifndef COMMANDFACTORY_H
#define COMMANDFACTORY_H

#include "command.h"

#include <QMap>
#include <QSharedPointer>

typedef QMap<QString, Command*(*)()> CommandMap;

class CommandFactory
{
public:
    static Command* create(const QString& command) {
        CommandMap* map = instance()->m_map;
        CommandMap::iterator it = map->find(command);
        if(it == map->end())
            return 0;
        return it.value()();
    }

    static CommandFactory* instance() {
        static CommandFactory* self = 0;
        if (self == 0)
            self = new CommandFactory();
        return self;
    }

    template<typename T>
    static void registerCommand(const QString& command);

    static QList<QString> commands()
    {
        return instance()->m_map->keys();
    }

private:
    CommandFactory():m_map(new CommandMap()){}
    template<typename T>  static Command *createCommand();

private:
    CommandMap *m_map;
};

template<typename T> void CommandFactory::registerCommand(const QString& command) {
    instance()->m_map->insert(command, &createCommand<T>);
}
template<typename T> Command *CommandFactory::createCommand() { return new T(); }
#endif // COMMANDFACTORY_H
