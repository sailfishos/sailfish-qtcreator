include(../qtcreator.pri)

TEMPLATE = app

CONFIG += console
CONFIG -= app_bundle

qtc_human_user_tool {
    DESTDIR = $$IDE_BIN_PATH
    target.path = $$INSTALL_BIN_PATH
} else {
    DESTDIR = $$IDE_LIBEXEC_PATH
    target.path  = $$INSTALL_LIBEXEC_PATH
}

INSTALLS += target

REL_PATH_TO_LIBS = $$relative_path($$IDE_LIBRARY_PATH, $$DESTDIR)
REL_PATH_TO_PLUGINS = $$relative_path($$IDE_PLUGIN_PATH, $$DESTDIR)
osx {
    QMAKE_LFLAGS += -Wl,-rpath,@executable_path/$$REL_PATH_TO_LIBS,-rpath,@executable_path/$$REL_PATH_TO_PLUGINS
} else {
    QMAKE_RPATHDIR += \$\$ORIGIN/$$REL_PATH_TO_LIBS
    QMAKE_RPATHDIR += \$\$ORIGIN/$$REL_PATH_TO_PLUGINS
}
include(rpath.pri)
