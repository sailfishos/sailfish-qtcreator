include(yaml_dependencies.pri)

INCLUDEPATH *= $$IDE_SOURCE_TREE/src/libs/mer/yaml
LIBS *= -l$$qtLibraryName(YAML)
