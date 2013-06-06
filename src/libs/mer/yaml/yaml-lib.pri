dll {
    DEFINES += YAML_LIB
} else {
    DEFINES += YAML_STATIC_LIB
}

INCLUDEPATH += \
    $$PWD \
    $$PWD/libyaml-stable \
    $$PWD/libyaml-stable/include

QT -= gui network


SOURCES += \
    $$PWD/libyaml-stable/src/api.c \
    $$PWD/libyaml-stable/src/dumper.c \
    $$PWD/libyaml-stable/src/emitter.c \
    $$PWD/libyaml-stable/src/loader.c \
    $$PWD/libyaml-stable/src/parser.c \
    $$PWD/libyaml-stable/src/reader.c \
    $$PWD/libyaml-stable/src/scanner.c \
    $$PWD/libyaml-stable/src/writer.c

HEADERS += \
    $$PWD/libyaml-stable/src/yaml_private.h \
    $$PWD/libyaml-stable/include/yaml.h \
    $$PWD/yaml_global.h
