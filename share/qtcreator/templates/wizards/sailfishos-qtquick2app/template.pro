# The name of your app
TARGET = %ProjectName%

CONFIG += sailfishapp

SOURCES += src/%ProjectName%.cpp

OTHER_FILES += qml/%ProjectName%.qml \
    qml/cover/CoverPage.qml \
    qml/pages/FirstPage.qml \
    qml/pages/SecondPage.qml \
    rpm/%ProjectName%.spec \
    rpm/%ProjectName%.yaml 
