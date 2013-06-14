TEMPLATE = lib
TARGET = Mer
PROVIDER = MerProject

QT += gui xmlpatterns

include(../../qtcreatorplugin.pri)

DEFINES += QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII

RESOURCES += \
    mer.qrc

FORMS += \
    meroptionswidget.ui \
    mersdkdetailswidget.ui \
    mersdkselectiondialog.ui \
    merdeviceconfigwizardgeneralpage.ui \
    merdeviceconfigurationwidget.ui \
    merdeploystep.ui \
    yamleditorwidget.ui

SOURCES += \
    merplugin.cpp \
    merqtversion.cpp \
    merqtversionfactory.cpp \
    mertoolchain.cpp \
    meroptionspage.cpp \
    merdevicefactory.cpp \
    merdeployconfigurationfactory.cpp \
    merdeviceconfigurationwizard.cpp \
    merdeviceconfigurationwizardsetuppages.cpp \
    merdevice.cpp \
    mersdkmanager.cpp \
    mersdk.cpp \
    mersftpdeployconfiguration.cpp \
    merrunconfiguration.cpp \
    merrunconfigurationfactory.cpp \
    merruncontrolfactory.cpp \
    meroptionswidget.cpp \
    mersdkdetailswidget.cpp \
    mersdkselectiondialog.cpp \
    meremulatorstartstep.cpp \
    merdeploystepfactory.cpp \
    merdeviceconfigurationwidget.cpp \
    mersshparser.cpp \
    mertarget.cpp \
    merconnection.cpp \
    merconnectionmanager.cpp \
    mervirtualboxmanager.cpp \
    mersdkkitinformation.cpp \
    mertargetsxmlparser.cpp \
    merdevicexmlparser.cpp \
    merprojectlistener.cpp \
    meryamlupdater.cpp \
    merbuildconfigurationprojectpathenvironmentvariablesetter.cpp \
    merdeploysteps.cpp \
    merdeployconfiguration.cpp \
    yamleditorfactory.cpp \
    yamleditorwidget.cpp \
    yamleditor.cpp \
    yamldocument.cpp \
    mertargetkitinformation.cpp

HEADERS += \
    merplugin.h \
    merconstants.h \
    merqtversion.h \
    merqtversionfactory.h \
    mertoolchain.h \
    meroptionspage.h \
    merdevicefactory.h \
    mertoolchainfactory.h \
    merdeployconfigurationfactory.h \
    merdeviceconfigurationwizard.h \
    merdeviceconfigurationwizardsetuppages.h \
    merdevice.h \
    mersdkmanager.h \
    mersdk.h \
    mersftpdeployconfiguration.h \
    merrunconfiguration.h \
    merrunconfigurationfactory.h \
    merruncontrolfactory.h \
    meroptionswidget.h \
    mersdkdetailswidget.h \
    mersdkselectiondialog.h \
    meremulatorstartstep.h \
    merdeploystepfactory.h \
    merdeviceconfigurationwidget.h \
    mersshparser.h \
    mertarget.h \
    merconnection.h \
    merconnectionmanager.h \
    mervirtualboxmanager.h \
    mersdkkitinformation.h \
    mertargetsxmlparser.h \
    merdevicexmlparser.h \
    merprojectlistener.h \
    meryamlupdater.h \
    merbuildconfigurationprojectpathenvironmentvariablesetter.h \
    merdeploysteps.h \
    merdeployconfiguration.h \
    yamleditorfactory.h \
    yamleditorwidget.h \
    yamleditor.h \
    yamldocument.h \
    mertargetkitinformation.h


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
