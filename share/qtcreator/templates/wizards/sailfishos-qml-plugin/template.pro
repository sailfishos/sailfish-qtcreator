TEMPLATE = lib
CONFIG += plugin
QT += qml

TARGET  = %{ProjectName}

@if %{IncludeCpp}
SOURCES += \\
        src/%{SrcPluginFile} \\
        src/%{SrcModelFile}

HEADERS += \\
        src/%{HdrPluginFile} \\
        src/%{HdrModelFile}

@endif
pluginfiles.files += \\
    imports/%{PluginName}/qmldir \\
    imports/%{PluginName}/%{QmlPluginFileName}

qml.files = plugin.qml
qml.path += /home/mersdk/share/projects/%{ProjectName}
target.path = $$[QT_INSTALL_QML]/%{PluginName}
pluginfiles.path += $$target.path

INSTALLS += target qml pluginfiles

CONFIG += install_ok  # Do not cargo-cult this!

OTHER_FILES += rpm/$${TARGET}.spec \\
    rpm/$${TARGET}.yaml \\
