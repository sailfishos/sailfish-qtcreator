import %{PluginName} 1.0 // import types from the plugin

%{PluginName} { // this class is defined in QML (imports/%{PluginName}/%{QmlPluginFileName})

@if %{IncludeCpp}
        %{PluginName}Model { // this class is defined in C++ (%{SrcFileName})
            id: templateClass
        }

@endif
    pageText: "This qml plugin template."
}

