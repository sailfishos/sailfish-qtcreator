TEMPLATE = lib
TARGET = Mer
PROVIDER = MerProject

QT += gui

include(../../qtcreatorplugin.pri)
include(mer_dependencies.pri)

DEFINES += QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII

RESOURCES += \
    mer.qrc

FORMS += \
    meroptionswidget.ui \
    sdkdetailswidget.ui \
    sdkselectiondialog.ui \
    merdeviceconfigwizardgeneralpage.ui \
    merdeviceconfigwizardcheckpreviouskeysetupcheckpage.ui \
    merdeviceconfigwizardreusekeyscheckpage.ui \
    merdeviceconfigwizardkeycreationpage.ui \
    merdeviceconfigwizarddevicetypepage.ui \
    merdeviceconfigurationwidget.ui

SOURCES += \
    merplugin.cpp \
    merqtversion.cpp \
    merqtversionfactory.cpp \
    mertoolchain.cpp \
    meroptionspage.cpp \
    sdkkitutils.cpp \
    merdevicefactory.cpp \
    merdeployconfigurationfactory.cpp \
    merdeviceconfigurationwizard.cpp \
    merdeviceconfigurationwizardsetuppages.cpp \
    merdevice.cpp \
    messageswindow.cpp \
    mersdkmanager.cpp \
    mersdk.cpp \
    sdktoolchainutils.cpp \
    sdktargetutils.cpp \
    mersftpdeployconfiguration.cpp \
    merrunconfiguration.cpp \
    merrunconfigurationfactory.cpp \
    merruncontrolfactory.cpp \
    sdkscriptsutils.cpp \
    meroptionswidget.cpp \
    sdkdetailswidget.cpp \
    sdkselectiondialog.cpp \
    mervirtualmachinemanager.cpp \
    meremulatorstartstep.cpp \
    merdeploystepfactory.cpp \
    merdeviceconfigurationwidget.cpp \
    mervirtualmachinebutton.cpp \
    mersshparser.cpp \
    virtualboxmanager.cpp

HEADERS += \
    merplugin.h\
    merconstants.h \
    merqtversion.h \
    merqtversionfactory.h \
    mertoolchain.h \
    meroptionspage.h \
    sdkkitutils.h \
    merdevicefactory.h \
    mertoolchainfactory.h \
    merdeployconfigurationfactory.h \
    merdeviceconfigurationwizard.h \
    merdeviceconfigurationwizardsetuppages.h \
    merdevice.h \
    messageswindow.h \
    mersdkmanager.h \
    mersdk.h \
    sdktoolchainutils.h \
    sdktargetutils.h \
    mersftpdeployconfiguration.h \
    merrunconfiguration.h \
    merrunconfigurationfactory.h \
    merruncontrolfactory.h \
    sdkscriptsutils.h \
    meroptionswidget.h \
    sdkdetailswidget.h \
    sdkselectiondialog.h \
    mervirtualmachinemanager.h \
    meremulatorstartstep.h \
    merdeploystepfactory.h \
    merdeviceconfigurationwidget.h \
    mervirtualmachinebutton.h \
    mersshparser.h \
    virtualboxmanager.h

SOURCES += \
    $$PWD/jollawelcomepage.cpp \
    $$PWD/mermode.cpp

HEADERS += \
    $$PWD/jollawelcomepage.h \
    $$PWD/mermode.h

contains(QT_CONFIG, webkit)|contains(QT_MODULES, webkit) {
    QT += webkit
    greaterThan(QT_MAJOR_VERSION, 4):QT += webkitwidgets
    SOURCES += $$PWD/mermanagementwebview.cpp
    HEADERS += $$PWD/mermanagementwebview.h
    FORMS += $$PWD/mermanagementwebview.ui
} else {
    DEFINES += QT_NO_WEBKIT
}
