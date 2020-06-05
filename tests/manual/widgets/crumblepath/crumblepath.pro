SOURCES += \
    tst_manual_widgets_crumblepath.cpp

RESOURCES += \
    ../themes.qrc

QTC_LIB_DEPENDS += \
    utils

QTC_PLUGIN_DEPENDS += \
    coreplugin

include(../../../auto/qttest.pri)
