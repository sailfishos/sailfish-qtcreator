/****************************************************************************
**
** Copyright (C) 2012-2015,2018 Jolla Ltd.
** Copyright (C) 2020 Open Mobile Platform LLC.
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

#ifndef COMMAND_H
#define COMMAND_H

#include <ssh/sshconnection.h>

#include <QCoreApplication>
#include <QStringList>
#include <QTimer>

class Command : public QObject
{
    Q_OBJECT

public:
    Command();
    virtual ~Command();
    int executeSfdk(const QStringList &arguments);
    QString sdkToolsPath() const;
    void setSdkToolsPath(const QString& path);
    QStringList arguments() const;
    void setArguments(const QStringList &args);

    virtual bool isValid() const;
    virtual QString name() const = 0;
    virtual int execute() = 0;

private:
    QStringList m_args;
    QString m_toolsPath;
    QSsh::SshConnectionParameters m_sshConnectionParams;
};

class CommandInvoker: public QObject
{
    Q_OBJECT
public:
    CommandInvoker(Command* command):m_command(command){
        Q_ASSERT(command);
        QTimer::singleShot(0, this, &CommandInvoker::execute);
    }
    ~CommandInvoker() override {}
private slots:
    void execute(){
        QCoreApplication::exit(m_command->execute());
    }
private:
    Command* m_command;
};

#endif // COMMAND_H
