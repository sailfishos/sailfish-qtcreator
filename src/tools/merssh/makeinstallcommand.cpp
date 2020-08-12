#include "makeinstallcommand.h"

MakeInstallCommand::MakeInstallCommand()
{

}

QString MakeInstallCommand::name() const
{
    return QLatin1String("make-install");
}

int MakeInstallCommand::execute()
{
    return executeSfdk(arguments());
}

bool MakeInstallCommand::isValid() const
{
    return Command::isValid() && !targetName().isEmpty() ;
}
