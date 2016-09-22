TEMPLATE = lib
TARGET = Mer
PROVIDER = MerProject

QT += gui xmlpatterns

include(../../qtcreatorplugin.pri)

DEFINES += QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII

RESOURCES += \
    mer.qrc

FORMS += \
    merdeploystep.ui \
    meremulatordevicewidget.ui \
    meremulatordevicewizardsshpage.ui \
    meremulatordevicewizardvmpage.ui \
    meremulatormodedialog.ui \
    mergeneraloptionswidget.ui \
    merhardwaredevicewidget.ui \
    merhardwaredevicewizardselectionpage.ui \
    merhardwaredevicewizardsetuppage.ui \
    meroptionswidget.ui \
    merrpmpackagingwidget.ui \
    mersdkdetailswidget.ui \
    mersdkselectiondialog.ui \

SOURCES += \
    merabstractvmstartstep.cpp \
    meraddvmstartbuildstepprojectlistener.cpp \
    merbuildstepfactory.cpp \
    merbuildsteps.cpp \
    merconnection.cpp \
    merconnectionmanager.cpp \
    merdeployconfiguration.cpp \
    merdeployconfigurationfactory.cpp \
    merdeploystepfactory.cpp \
    merdeploysteps.cpp \
    merdevice.cpp \
    merdevicefactory.cpp \
    merdevicexmlparser.cpp \
    meremulatordevice.cpp \
    meremulatordevicetester.cpp \
    meremulatordevicewidget.cpp \
    meremulatordevicewizard.cpp \
    meremulatordevicewizardpages.cpp \
    meremulatormodedialog.cpp \
    mergeneraloptionspage.cpp \
    mergeneraloptionswidget.cpp \
    merhardwaredevice.cpp \
    merhardwaredevicewidget.cpp \
    merhardwaredevicewizard.cpp \
    merhardwaredevicewizardpages.cpp \
    mermode.cpp \
    meroptionspage.cpp \
    meroptionswidget.cpp \
    merplugin.cpp \
    merprojectlistener.cpp \
    merqtversion.cpp \
    merqtversionfactory.cpp \
    merrpminstaller.cpp \
    merrpmpackagingstep.cpp \
    merrpmpackagingwidget.cpp \
    merrpmvalidationparser.cpp \
    merrunconfiguration.cpp \
    merrunconfigurationfactory.cpp \
    merruncontrolfactory.cpp \
    mersdk.cpp \
    mersdkdetailswidget.cpp \
    mersdkkitinformation.cpp \
    mersdkmanager.cpp \
    mersdkselectiondialog.cpp \
    mersettings.cpp \
    mersftpdeployconfiguration.cpp \
    mersshkeydeploymentdialog.cpp \
    mersshparser.cpp \
    mertarget.cpp \
    mertargetkitinformation.cpp \
    mertargetsxmlparser.cpp \
    mertoolchain.cpp \
    meruploadandinstallrpmsteps.cpp \
    mervirtualboxmanager.cpp \

HEADERS += \
    merabstractvmstartstep.h \
    meraddvmstartbuildstepprojectlistener.h \
    merbuildstepfactory.h \
    merbuildsteps.h \
    merconnection.h \
    merconnectionmanager.h \
    merconstants.h \
    merdeployconfiguration.h \
    merdeployconfigurationfactory.h \
    merdeploystepfactory.h \
    merdeploysteps.h \
    merdevice.h \
    merdevicefactory.h \
    merdevicexmlparser.h \
    meremulatordevice.h \
    meremulatordevicetester.h \
    meremulatordevicewidget.h \
    meremulatordevicewizard.h \
    meremulatordevicewizardpages.h \
    meremulatormodedialog.h \
    mergeneraloptionspage.h \
    mergeneraloptionswidget.h \
    merhardwaredevice.h \
    merhardwaredevicewidget.h \
    merhardwaredevicewizard.h \
    merhardwaredevicewizardpages.h \
    mericons.h \
    mermode.h \
    meroptionspage.h \
    meroptionswidget.h \
    merplugin.h \
    merprojectlistener.h \
    merqtversion.h \
    merqtversionfactory.h \
    merrpminstaller.h \
    merrpmpackagingstep.h \
    merrpmpackagingwidget.h \
    merrpmvalidationparser.h \
    merrunconfiguration.h \
    merrunconfigurationfactory.h \
    merruncontrolfactory.h \
    mersdk.h \
    mersdkdetailswidget.h \
    mersdkkitinformation.h \
    mersdkmanager.h \
    mersdkselectiondialog.h \
    mersettings.h \
    mersftpdeployconfiguration.h \
    mersshkeydeploymentdialog.h \
    mersshparser.h \
    mertarget.h \
    mertargetkitinformation.h \
    mertargetsxmlparser.h \
    mertoolchain.h \
    mertoolchainfactory.h \
    meruploadandinstallrpmsteps.h \
    mervirtualboxmanager.h \

contains(QT_CONFIG, webkit)|contains(QT_MODULES, webkit) {
    QT += webkit
    greaterThan(QT_MAJOR_VERSION, 4):QT += webkitwidgets
    SOURCES += $$PWD/mermanagementwebview.cpp
    HEADERS += $$PWD/mermanagementwebview.h
    FORMS += $$PWD/mermanagementwebview.ui
} else {
    DEFINES += QT_NO_WEBKIT
}
