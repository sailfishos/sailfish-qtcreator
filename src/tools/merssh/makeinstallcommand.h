#ifndef MAKEINSTALLCOMMAND_H
#define MAKEINSTALLCOMMAND_H

#include "command.h"

class MakeInstallCommand : public Command
{
    Q_OBJECT
public:
    MakeInstallCommand();
    QString name() const override;
    int execute() override;
    bool isValid() const override;
};

#endif // MAKEINSTALLCOMMAND_H
