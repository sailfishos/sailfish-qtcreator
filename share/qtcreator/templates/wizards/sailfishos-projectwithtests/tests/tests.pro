TARGET = %{ProjectName}-app-tests

QT += testlib

CONFIG += sailfishapp qt c++11

INCLUDEPATH += ../app/

include(../app/app.pri)

HEADERS += \\
    src/testsample.h

SOURCES += \\
    src/testsample.cpp \\
    src/main.cpp
