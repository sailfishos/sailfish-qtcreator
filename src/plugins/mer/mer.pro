TEMPLATE = lib
TARGET = Mer
PROVIDER = MerProject

QT += gui xmlpatterns

include(../../qtcreatorplugin.pri)

DEFINES += MER_LIBRARY

*-g++*:QMAKE_CXXFLAGS += -Wall -Werror

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
    merhardwaredevicewidgetauthorizedialog.ui \
    merhardwaredevicewizardselectionpage.ui \
    merhardwaredevicewizardsetuppage.ui \
    meroptionswidget.ui \
    merrpminfo.ui \
    merrpmpackagingwidget.ui \
    merrpmvalidationstepconfigwidget.ui \
    merrunconfigurationaspectqmllivedetailswidget.ui \
    mersdkdetailswidget.ui \
    mersdkselectiondialog.ui \
    meremulatormodeoptionswidget.ui \
    meremulatormodedetailswidget.ui

SOURCES += \
    merabstractvmstartstep.cpp \
    meraddvmstartbuildstepprojectlistener.cpp \
    merbuildsteps.cpp \
    merconnection.cpp \
    merconnectionmanager.cpp \
    merdeployconfiguration.cpp \
    merdeploysteps.cpp \
    merdevice.cpp \
    merdevicedebugsupport.cpp \
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
    mericons.cpp \
    merlogging.cpp \
    mermode.cpp \
    meroptionspage.cpp \
    meroptionswidget.cpp \
    merplugin.cpp \
    merprojectlistener.cpp \
    merqmllivebenchmanager.cpp \
    merqmlrunconfiguration.cpp \
    merqmlrunconfigurationfactory.cpp \
    merqmlrunconfigurationwidget.cpp \
    merqtversion.cpp \
    merqtversionfactory.cpp \
    merrpminstaller.cpp \
    merrpmpackagingstep.cpp \
    merrpmpackagingwidget.cpp \
    merrpmvalidationparser.cpp \
    merrunconfiguration.cpp \
    merrunconfigurationaspect.cpp \
    merrunconfigurationfactory.cpp \
    mersdk.cpp \
    mersdkdetailswidget.cpp \
    mersdkkitinformation.cpp \
    mersdkmanager.cpp \
    mersdkselectiondialog.cpp \
    mersettings.cpp \
    mersshkeydeploymentdialog.cpp \
    mersshparser.cpp \
    mertarget.cpp \
    mertargetkitinformation.cpp \
    mertargetsxmlparser.cpp \
    mertoolchain.cpp \
    meruploadandinstallrpmsteps.cpp \
    mervirtualboxmanager.cpp \
    merdevicemodelcombobox.cpp \
    meremulatormodeoptionspage.cpp \
    meremulatormodeoptionswidget.cpp \
    meremulatormodedetailswidget.cpp

HEADERS += \
    merabstractvmstartstep.h \
    meraddvmstartbuildstepprojectlistener.h \
    merbuildsteps.h \
    merconnection.h \
    merconnectionmanager.h \
    merconstants.h \
    merdeployconfiguration.h \
    merdeploysteps.h \
    merdevice.h \
    merdevicedebugsupport.h \
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
    merlogging.h \
    mermode.h \
    meroptionspage.h \
    meroptionswidget.h \
    merplugin.h \
    merprojectlistener.h \
    merqmllivebenchmanager.h \
    merqmlrunconfiguration.h \
    merqmlrunconfigurationfactory.h \
    merqmlrunconfigurationwidget.h \
    merqtversion.h \
    merqtversionfactory.h \
    merrpminstaller.h \
    merrpmpackagingstep.h \
    merrpmpackagingwidget.h \
    merrpmvalidationparser.h \
    merrunconfiguration.h \
    merrunconfigurationaspect.h \
    merrunconfigurationfactory.h \
    mersdk.h \
    mersdkdetailswidget.h \
    mersdkkitinformation.h \
    mersdkmanager.h \
    mersdkselectiondialog.h \
    mersettings.h \
    mersshkeydeploymentdialog.h \
    mersshparser.h \
    mertarget.h \
    mertargetkitinformation.h \
    mertargetsxmlparser.h \
    mertoolchain.h \
    mertoolchainfactory.h \
    meruploadandinstallrpmsteps.h \
    mervirtualboxmanager.h \
    merdevicemodelcombobox.h \
    meremulatormodeoptionspage.h \
    meremulatormodeoptionswidget.h \
    meremulatormodedetailswidget.h

contains(QT_CONFIG, webkit)|contains(QT_MODULES, webkit) {
    QT += webkit
    greaterThan(QT_MAJOR_VERSION, 4):QT += webkitwidgets
    SOURCES += $$PWD/mermanagementwebview.cpp
    HEADERS += $$PWD/mermanagementwebview.h
    FORMS += $$PWD/mermanagementwebview.ui
} else {
    DEFINES += QT_NO_WEBKIT
}
