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
    merbuildconfigurationwidget.ui \
    merbuildenginedetailswidget.ui \
    merbuildengineoptionswidget.ui \
    merdeploystep.ui \
    meremulatordetailswidget.ui \
    meremulatormodedialog.ui \
    meremulatoroptionswidget.ui \
    mergeneraloptionswidget.ui \
    merhardwaredevicewidget.ui \
    merhardwaredevicewizardselectionpage.ui \
    merhardwaredevicewizardconnectiontestpage.ui \
    merhardwaredevicewizardsetuppage.ui \
    merrpminfo.ui \
    merrpmvalidationstepconfigwidget.ui \
    merrunconfigurationaspectqmllivedetailswidget.ui \
    meremulatormodeoptionswidget.ui \
    meremulatormodedetailswidget.ui \
    mertargetmanagementpage.ui \
    mertargetmanagementpackagespage.ui \
    mertargetmanagementprogresspage.ui \
    mervirtualmachinesettingswidget.ui \
    mervmselectiondialog.ui \

SOURCES += \
    merabstractvmstartstep.cpp \
    merbuildconfigurationaspect.cpp \
    merbuildenginedetailswidget.cpp \
    merbuildengineoptionspage.cpp \
    merbuildengineoptionswidget.cpp \
    merbuildsteps.cpp \
    mercompilationdatabasebuildconfiguration.cpp \
    merconnectionmanager.cpp \
    mercustomrunconfiguration.cpp \
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
    merplugin.cpp \
    merprojectlistener.cpp \
    merqmllivebenchmanager.cpp \
    merqmlrunconfiguration.cpp \
    merqmltoolingsupport.cpp \
    merqtversion.cpp \
    merrpminstaller.cpp \
    merrpmvalidationparser.cpp \
    merrunconfiguration.cpp \
    merrunconfigurationaspect.cpp \
    mersdkkitaspect.cpp \
    mersdkmanager.cpp \
    mersettings.cpp \
    mersigninguserselectiondialog.cpp \
    mersshparser.cpp \
    mertargetmanagementdialog.cpp \
    mertoolchain.cpp \
    mervmconnectionui.cpp \
    mervmselectiondialog.cpp \
    merdevicemodelcombobox.cpp \
    meremulatormodeoptionspage.cpp \
    meremulatormodeoptionswidget.cpp \
    meremulatormodedetailswidget.cpp \
    mervirtualmachinesettingswidget.cpp \
    mercmakebuildconfiguration.cpp \
    merqmakebuildconfiguration.cpp

HEADERS += \
    merabstractvmstartstep.h \
    merbuildconfigurationaspect.h \
    merbuildenginedetailswidget.h \
    merbuildengineoptionspage.h \
    merbuildengineoptionswidget.h \
    merbuildsteps.h \
    mercompilationdatabasebuildconfiguration.h \
    merconnectionmanager.h \
    merconstants.h \
    mercustomrunconfiguration.h \
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
    merplugin.h \
    merprojectlistener.h \
    merqmllivebenchmanager.h \
    merqmlrunconfiguration.h \
    merqmltoolingsupport.h \
    merqtversion.h \
    merrpminstaller.h \
    merrpmvalidationparser.h \
    merrunconfiguration.h \
    merrunconfigurationaspect.h \
    mersdkkitaspect.h \
    mersdkmanager.h \
    mersettings.h \
    mersigninguserselectiondialog.h \
    mersshparser.h \
    mertargetmanagementdialog.h \
    mertoolchain.h \
    mervmconnectionui.h \
    mervmselectiondialog.h \
    merdevicemodelcombobox.h \
    meremulatormodeoptionspage.h \
    meremulatormodeoptionswidget.h \
    meremulatormodedetailswidget.h \
    mervirtualmachinesettingswidget.h \
    mercmakebuildconfiguration.h \
    merqmakebuildconfiguration.h
