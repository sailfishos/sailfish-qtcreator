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
    merdeploystep.ui \
    yamleditorwidget.ui \
    meremulatordevicewidget.ui \
    merhardwaredevicewidget.ui \
    merhardwaredevicewizardgeneralpage.ui \
    meremulatordevicewizardvmpage.ui \
    meremulatordevicewizardsshpage.ui \
    merrpmpackagingwidget.ui \
    merhardwaredevicewizardkeypage.ui

SOURCES += \
    merplugin.cpp \
    merqtversion.cpp \
    merqtversionfactory.cpp \
    mertoolchain.cpp \
    meroptionspage.cpp \
    merdevicefactory.cpp \
    merdeployconfigurationfactory.cpp \
    mersdkmanager.cpp \
    mersdk.cpp \
    mersftpdeployconfiguration.cpp \
    merrunconfiguration.cpp \
    merrunconfigurationfactory.cpp \
    merruncontrolfactory.cpp \
    meroptionswidget.cpp \
    mersdkdetailswidget.cpp \
    mersdkselectiondialog.cpp \
    merdeploystepfactory.cpp \
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
    merdeploysteps.cpp \
    merdeployconfiguration.cpp \
    yamleditorfactory.cpp \
    yamleditorwidget.cpp \
    yamleditor.cpp \
    yamldocument.cpp \
    mertargetkitinformation.cpp \
    meremulatordevice.cpp \
    meremulatordevicewidget.cpp \
    merhardwaredevice.cpp \
    merhardwaredevicewidget.cpp \
    meremulatordevicewizard.cpp \
    merhardwaredevicewizard.cpp \
    meremulatordevicewizardpages.cpp \
    merconnectionprompt.cpp \
    meractionmanager.cpp \
    merrpmpackagingstep.cpp \
    merrpmpackagingwidget.cpp \
    merrpminstaller.cpp \
    meruploadandinstallrpmsteps.cpp \
    merhardwaredevicewizardpages.cpp \
    mersshkeydeploymentdialog.cpp


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
    mersdkmanager.h \
    mersdk.h \
    mersftpdeployconfiguration.h \
    merrunconfiguration.h \
    merrunconfigurationfactory.h \
    merruncontrolfactory.h \
    meroptionswidget.h \
    mersdkdetailswidget.h \
    mersdkselectiondialog.h \
    merdeploystepfactory.h \
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
    merdeploysteps.h \
    merdeployconfiguration.h \
    yamleditorfactory.h \
    yamleditorwidget.h \
    yamleditor.h \
    yamldocument.h \
    mertargetkitinformation.h \
    meremulatordevice.h \
    meremulatordevicewidget.h \
    merhardwaredevice.h \
    merhardwaredevicewidget.h \
    meremulatordevicewizard.h \
    merhardwaredevicewizard.h \
    meremulatordevicewizardpages.h \
    merconnectionprompt.h \
    meractionmanager.h \
    merrpmpackagingstep.h \
    merrpmpackagingwidget.h \
    merrpminstaller.h \
    meruploadandinstallrpmsteps.h \
    merhardwaredevicewizardpages.h \
    mersshkeydeploymentdialog.h

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
