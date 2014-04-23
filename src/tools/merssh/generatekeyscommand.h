#ifndef GENERATEKEYSCOMMAND_H
#define GENERATEKEYSCOMMAND_H

#include "command.h"

class GenerateKeysCommand: public Command
{
public:
    GenerateKeysCommand();
    QString name() const;
    int execute();
    bool isValid() const;
private:
    bool parseArguments();
    QString unquote(const QString &args);
    QString m_privateKeyFileName;
    QString m_publicKeyFileName;
};

#endif // GENERATEKEYSCOMMAND_H
