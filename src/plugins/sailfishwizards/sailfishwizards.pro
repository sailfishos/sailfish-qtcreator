include(../../qtcreatorplugin.pri)

DEFINES += SAILFISHWIZARDS_LIBRARY

# SailfishWizards files

SOURCES += \
    common.cpp \
    sailfishwizardsplugin.cpp \
    desktopwizardfactory.cpp \
    desktopwizardpages.cpp \
    desktopeditorfactory.cpp \
    desktopeditor.cpp \
    desktopeditorwidget.cpp \
    desktopdocument.cpp \
    dependencylistmodel.cpp \
    dependencymodel.cpp \
    sailfishprojectdependencies.cpp \
    wizardpage.cpp \
    cppclasswizardfactory.cpp \
    cppclasswizardpages.cpp \
    fileinfopage.cpp \
    qmlwizardpage.cpp \
    qmlwizardfactory.cpp \
    unittestwizardfactory.cpp \
    unittestwizardpages.cpp \
    javascriptlibrarywizardfactory.cpp \

HEADERS += \
    common.h \
    sailfishwizards_global.h \
    sailfishwizardsconstants.h \
    sailfishwizardsplugin.h \
    desktopwizardfactory.h \
    desktopwizardpages.h \
    desktopeditorfactory.h \
    desktopeditor.h \
    desktopeditorwidget.h \
    desktopdocument.h \
    dependencylistmodel.h \
    dependencymodel.h \
    sailfishprojectdependencies.h \
    wizardpage.h \
    cppclasswizardfactory.h \
    cppclasswizardpages.h \
    fileinfopage.h \
    qmlwizardpage.h \
    qmlwizardfactory.h \
    unittestwizardfactory.h \
    unittestwizardpages.h \
    javascriptlibrarywizardfactory.h \

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
    forms/desktopfilesettingpage.ui \
    forms/desktopselectpage.ui \
    forms/desktopiconpage.ui \
    forms/cppclassfileselectpage.ui \
    forms/cppclassoptionpage.ui \
    forms/jsfileselectpage.ui \
    forms/choicefiledialog.ui \
    forms/externallibrarydialog.ui \
    forms/externallibrarypage.ui \
    forms/qmlfileselectpage.ui \
    forms/qmllocalimportpage.ui \
    forms/qmloptionpage.ui \
    forms/unittestfileselectpage.ui \

RESOURCES += \
    resources.qrc \
