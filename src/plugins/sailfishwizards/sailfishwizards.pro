include(../../qtcreatorplugin.pri)

DEFINES += SAILFISHWIZARDS_LIBRARY

# SailfishWizards files

SOURCES += \
    common.cpp \
    sailfishwizardsplugin.cpp \
    dependencylistmodel.cpp \
    dependencymodel.cpp \
    sailfishprojectdependencies.cpp \
    wizardpage.cpp \

HEADERS += \
    common.h \
    sailfishwizards_global.h \
    sailfishwizardsconstants.h \
    sailfishwizardsplugin.h \
    dependencylistmodel.h \
    dependencymodel.h \
    sailfishprojectdependencies.h \
    wizardpage.h \

###### If the plugin can be depended upon by other plugins, this code needs to be outsourced to
###### <dirname>_dependencies.pri, where <dirname> is the name of the directory containing the
###### plugin's sources.

QTC_PLUGIN_NAME = SailfishWizards
QTC_LIB_DEPENDS += \
    # nothing here at this time

QTC_PLUGIN_DEPENDS += \
    coreplugin \
    projectexplorer \

QTC_PLUGIN_RECOMMENDS += \
    # optional plugin dependencies. nothing here at this time

FORMS += \
    forms/choicefiledialog.ui \
    forms/externallibrarydialog.ui \
    forms/externallibrarypage.ui \
