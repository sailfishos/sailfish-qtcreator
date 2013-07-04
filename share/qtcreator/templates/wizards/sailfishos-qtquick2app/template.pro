# The name of your app
TARGET = template

# C++ sources
SOURCES += main.cpp

# C++ headers
HEADERS +=

# QML files and folders
qml.files = *.qml pages cover main.qml

# The .desktop file
desktop.files = template.desktop

# Please do not modify the following line.
include(sailfishapplication/sailfishapplication.pri)

OTHER_FILES = \
    rpm/template.yaml \
    rpm/template.spec
