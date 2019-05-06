#include <QtQml>
#include "%{HdrPluginFile}"
#include "%{HdrModelFile}"


void %{PluginClassName}::registerTypes(const char *uri)
{
    qmlRegisterType<%{ModelClassName}>(uri, 1, 0, "%{ModelClassName}");
}


