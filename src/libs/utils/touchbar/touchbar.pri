HEADERS += $$PWD/touchbar.h

macos:!equals(QMAKE_MACOSX_DEPLOYMENT_TARGET, "10.10") {
    HEADERS += \
        $$PWD/touchbar_mac_p.h \
        $$PWD/touchbar_appdelegate_mac_p.h

    OBJECTIVE_SOURCES += \
        $$PWD/touchbar_mac.mm \
        $$PWD/touchbar_appdelegate_mac.mm

    QT += macextras
    LIBS += -framework Foundation -framework AppKit
} else {
    SOURCES += $$PWD/touchbar.cpp
}

