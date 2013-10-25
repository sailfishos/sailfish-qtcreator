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

#ifndef MERACTIONMANAGER_H
#define MERACTIONMANAGER_H

#include <QObject>
namespace Core {
class ActionContainer;
}

namespace Mer {
namespace Internal {

class Prompt;

class MerActionManager : public QObject
{
    Q_OBJECT
public:
    static MerActionManager *instance();
    static Prompt* createStartPrompt(const QString& virtualMachine);
    static Prompt* createClosePrompt(const QString& virtualMachine);

    ~MerActionManager();
private:
    MerActionManager();

private slots:
    void updateActions();
    void handleOptionsTriggered();
    void handleStartTriggered();
    void handleStopTriggered();
private:
    void setupActions();
private:
    static MerActionManager *m_instance;
    Core::ActionContainer *m_menu;
    Core::ActionContainer *m_startMenu;
    Core::ActionContainer *m_stopMenu;
friend class MerPlugin;
};

class Prompt: public QObject
{
    Q_OBJECT
private:
    Prompt();
private slots:
    void show();
signals:
    void closed(const QString& vm, bool accepted);
private:
    QString m_title;
    QString m_text;
    QString m_vm;
friend class MerActionManager;
};

}
}
#endif // MERACTIONMANAGER_H
