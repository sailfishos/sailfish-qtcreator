#ifndef %{PluginHdrGuard}
#define %{PluginHdrGuard}

#include <QtQml/QQmlExtensionPlugin>

class %{PluginClassName} : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "%{PluginName}")

public:
    void registerTypes(const char *uri) override;
};

#endif // %{PluginHdrGuard}
