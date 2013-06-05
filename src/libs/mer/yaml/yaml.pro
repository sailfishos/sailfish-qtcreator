TEMPLATE = lib
TARGET = YAML
QT -= gui network

include(../../../qtcreatorlibrary.pri)

INCLUDEPATH += \
    libyaml-stable \
    libyaml-stable/include

SOURCES += \
    libyaml-stable/src/api.c \
    libyaml-stable/src/dumper.c \
    libyaml-stable/src/emitter.c \
    libyaml-stable/src/loader.c \
    libyaml-stable/src/parser.c \
    libyaml-stable/src/reader.c \
    libyaml-stable/src/scanner.c \
    libyaml-stable/src/writer.c


HEADERS += \
    libyaml-stable/src/yaml_private.h \
    libyaml-stable/include/yaml.h
