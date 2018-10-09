import qbs.base 1.0

import "../QtcPlugin.qbs" as QtcPlugin

QtcPlugin {
    name: "SailfishOS"

    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "QmakeProjectManager" }
    Depends { name: "RemoteLinux" }
    Depends { name: "QtcSsh" }
    Depends { name: "Qt"; submodules: ["widgets", "webkit"] }
    Depends { name: "cpp" }

    files: [
        "meroptionswidget.ui",
        "mersdkdetailswidget.ui",
        "mersdkselectiondialog.ui",
        "merdeviceconfigwizardgeneralpage.ui",
        "merdeviceconfigwizardcheckpreviouskeysetupcheckpage.ui",
        "merdeviceconfigwizardreusekeyscheckpage.ui",
        "merdeviceconfigwizardkeycreationpage.ui",
        "merdeviceconfigwizarddevicetypepage.ui",
        "merdeviceconfigurationwidget.ui",
        "merplugin.cpp",
        "merqtversion.cpp",
        "merqtversionfactory.cpp",
        "mertoolchain.cpp",
        "meroptionspage.cpp",
        "merdevicefactory.cpp",
        "merdeployconfigurationfactory.cpp",
        "merdeviceconfigurationwizard.cpp",
        "merdeviceconfigurationwizardsetuppages.cpp",
        "merdevice.cpp",
        "mersdkmanager.cpp",
        "mersdk.cpp",
        "mersftpdeployconfiguration.cpp",
        "merrunconfiguration.cpp",
        "merrunconfigurationfactory.cpp",
        "merruncontrolfactory.cpp",
        "meroptionswidget.cpp",
        "mersdkdetailswidget.cpp",
        "mersdkselectiondialog.cpp",
        "mervirtualmachinemanager.cpp",
        "meremulatorstartstep.cpp",
        "merdeploystepfactory.cpp",
        "merdeviceconfigurationwidget.cpp",
        "mervirtualmachinebutton.cpp",
        "mersshparser.cpp",
        "virtualboxmanager.cpp",
        "mertarget.cpp",
        "merplugin.h",
        "merconstants.h",
        "merqtversion.h",
        "merqtversionfactory.h",
        "mertoolchain.h",
        "meroptionspage.h",
        "merdevicefactory.h",
        "mertoolchainfactory.h",
        "merdeployconfigurationfactory.h",
        "merdeviceconfigurationwizard.h",
        "merdeviceconfigurationwizardsetuppages.h",
        "merdevice.h",
        "mersdkmanager.h",
        "mersdk.h",
        "mersftpdeployconfiguration.h",
        "merrunconfiguration.h",
        "merrunconfigurationfactory.h",
        "merruncontrolfactory.h",
        "meroptionswidget.h",
        "mersdkdetailswidget.h",
        "mersdkselectiondialog.h",
        "mervirtualmachinemanager.h",
        "meremulatorstartstep.h",
        "merdeploystepfactory.h",
        "merdeviceconfigurationwidget.h",
        "mervirtualmachinebutton.h",
        "mersshparser.h",
        "virtualboxmanager.h",
        "mertarget.h",
        "merconnection.cpp",
        "merconnection.h",
        "merconnectionmanager.cpp",
        "merconnectionmanager.h",
        "mericons.h",
        "mermode.cpp",
        "mermode.h",
        "mermanagementwebview.cpp",
        "mermanagementwebview.h",
        "mermanagementwebview.ui"
    ]
}
