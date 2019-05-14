#ifndef GENERATEKEYSCOMMAND_H
#define GENERATEKEYSCOMMAND_H

#include "command.h"

class GenerateKeysCommand: public Command
{
    Q_OBJECT

public:
    GenerateKeysCommand();
    QString name() const override;
    int execute() override;
    bool isValid() const override;
private:
    bool parseArguments();
    QString unquote(const QString &args);
    QString m_privateKeyFileName;
};

#endif // GENERATEKEYSCOMMAND_H
