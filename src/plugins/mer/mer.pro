TEMPLATE = lib
TARGET = SailfishOS
PROVIDER = MerProject

QT += gui xmlpatterns

include(../../qtcreatorplugin.pri)

DEFINES += MER_LIBRARY
DEFINES += QT_NO_URL_CAST_FROM_STRING

*-g++*:QMAKE_CXXFLAGS += -Wall -Werror

RESOURCES += \
    mer.qrc

FORMS += \
    merdeploystep.ui \
    meremulatordetailswidget.ui \
    meremulatormodedialog.ui \
    meremulatoroptionswidget.ui \
    mergeneraloptionswidget.ui \
    merhardwaredevicewidget.ui \
    merhardwaredevicewidgetauthorizedialog.ui \
    merhardwaredevicewizardselectionpage.ui \
    merhardwaredevicewizardsetuppage.ui \
    meroptionswidget.ui \
    merrpminfo.ui \
    merrpmvalidationstepconfigwidget.ui \
    merrunconfigurationaspectqmllivedetailswidget.ui \
    mersdkdetailswidget.ui \
    meremulatormodeoptionswidget.ui \
    meremulatormodedetailswidget.ui \
    mervirtualmachinesettingswidget.ui \
    mervmselectiondialog.ui \

SOURCES += \
    merabstractvmstartstep.cpp \
    merbuildsteps.cpp \
    merconnectionmanager.cpp \
    merdeployconfiguration.cpp \
    merdeploysteps.cpp \
    merdevice.cpp \
    merdevicedebugsupport.cpp \
    merdevicefactory.cpp \
    meremulatordetailswidget.cpp \
    meremulatordevice.cpp \
    meremulatordevicetester.cpp \
    meremulatormodedialog.cpp \
    meremulatoroptionspage.cpp \
    meremulatoroptionswidget.cpp \
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
    merrpmvalidationparser.cpp \
    merrunconfiguration.cpp \
    merrunconfigurationaspect.cpp \
    merrunconfigurationfactory.cpp \
    mersdkdetailswidget.cpp \
    mersdkkitinformation.cpp \
    mersdkmanager.cpp \
    mersettings.cpp \
    mersshkeydeploymentdialog.cpp \
    mersshparser.cpp \
    mertoolchain.cpp \
    mervmconnectionui.cpp \
    mervmselectiondialog.cpp \
    merdevicemodelcombobox.cpp \
    meremulatormodeoptionspage.cpp \
    meremulatormodeoptionswidget.cpp \
    meremulatormodedetailswidget.cpp \
    mervirtualmachinesettingswidget.cpp \
    merbuildconfiguration.cpp

HEADERS += \
    merabstractvmstartstep.h \
    merbuildsteps.h \
    merconnectionmanager.h \
    merconstants.h \
    merdeployconfiguration.h \
    merdeploysteps.h \
    merdevice.h \
    merdevicedebugsupport.h \
    merdevicefactory.h \
    meremulatordetailswidget.h \
    meremulatordevice.h \
    meremulatordevicetester.h \
    meremulatormodedialog.h \
    meremulatoroptionspage.h \
    meremulatoroptionswidget.h \
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
    merrpmvalidationparser.h \
    merrunconfiguration.h \
    merrunconfigurationaspect.h \
    merrunconfigurationfactory.h \
    mersdkdetailswidget.h \
    mersdkkitinformation.h \
    mersdkmanager.h \
    mersettings.h \
    mersshkeydeploymentdialog.h \
    mersshparser.h \
    mertoolchain.h \
    mertoolchainfactory.h \
    mervmconnectionui.h \
    mervmselectiondialog.h \
    merdevicemodelcombobox.h \
    meremulatormodeoptionspage.h \
    meremulatormodeoptionswidget.h \
    meremulatormodedetailswidget.h \
    mervirtualmachinesettingswidget.h \
    merbuildconfiguration.h

contains(QT_CONFIG, webkit)|contains(QT_MODULES, webkit) {
    QT += webkit
    greaterThan(QT_MAJOR_VERSION, 4):QT += webkitwidgets
    SOURCES += $$PWD/mermanagementwebview.cpp
    HEADERS += $$PWD/mermanagementwebview.h
    FORMS += $$PWD/mermanagementwebview.ui
} else {
    DEFINES += QT_NO_WEBKIT
}
