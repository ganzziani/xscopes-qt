QT += core gui serialport concurrent
QT += network

TARGET = xscope
TEMPLATE = app

# The application version, written down only here. qmake puts it in the Windows version
# resource it generates (with the QMAKE_TARGET_* strings and RC_ICONS below) and in the macOS
# Info.plist; main.cpp gets it as APP_VERSION_STRING; build-installer.bat reads this line.
# 1.0.0.x was the Qt 4/5 series. The Qt 6 port is 1.1.x.
VERSION = 1.1.0
DEFINES += APP_VERSION_STRING=\\\"$$VERSION\\\"

QMAKE_TARGET_COMPANY = Gabotronics
QMAKE_TARGET_PRODUCT = XScopes Interface
QMAKE_TARGET_DESCRIPTION = XScopes Interface
QMAKE_TARGET_COPYRIGHT = Copyright (C) 2013-2026 Gabotronics. GPL-3.0-or-later.
QMAKE_TARGET_INTERNALNAME = xscope
QMAKE_TARGET_ORIGINAL_FILENAME = xscope.exe
RC_ICONS = Bitmaps/GT-icon.ico

SOURCES += main.cpp\
    xprotolabinterface.cpp \
    qcustomplot.cpp \
    libusbdevice.cpp \
    fft.cpp \
    complex.cpp \
    customtheme.cpp \
    serialportconnection.cpp \
    customcolors.cpp \
    filedownloader.cpp \
    bootloader.cpp \
    dfuprogrammerqt.cpp \
    third_party/dfu-programmer/arguments.cpp \
    third_party/dfu-programmer/atmel.cpp \
    third_party/dfu-programmer/commands.cpp \
    third_party/dfu-programmer/dfu.cpp \
    third_party/dfu-programmer/intel_hex.cpp \
    third_party/dfu-programmer/libdfu.cpp \
    third_party/dfu-programmer/stm32.cpp \
    third_party/dfu-programmer/util.cpp

HEADERS  += xprotolabinterface.h \
    qcustomplot.h \
    libusb.h \
    libusbdevice.h \
    libusbdeviceinfo.h \
    fft.h \
    complex.h \
    sniffer.h \
    customtheme.h \
    serialportconnection.h \
    customcolors.h \
    filedownloader.h \
    bootloader.h \
    dfuprogrammerqt.h \
    markerpixmap.h \
    fittedlabel.h \
    persistenceitem.h \
    third_party/dfu-programmer/arguments.h \
    third_party/dfu-programmer/atmel.h \
    third_party/dfu-programmer/commands.h \
    third_party/dfu-programmer/compat-msvc.h \
    third_party/dfu-programmer/config.h \
    third_party/dfu-programmer/dfu-device.h \
    third_party/dfu-programmer/dfu.h \
    third_party/dfu-programmer/dfu_redirect.h \
    third_party/dfu-programmer/intel_hex.h \
    third_party/dfu-programmer/libdfu.h \
    third_party/dfu-programmer/stm32.h \
    third_party/dfu-programmer/util.h

FORMS += xprotolabinterface.ui \
    customtheme.ui \
    bootloader.ui

RESOURCES += \
    xprotolabinterface.qrc

INCLUDEPATH += $$PWD/libs $$PWD
DEPENDPATH += $$PWD/libs $$PWD

greaterThan(QT_MAJOR_VERSION, 4):QT += widgets
greaterThan(QT_MAJOR_VERSION, 4):QT += printsupport

win32: PRE_TARGETDEPS += $$PWD/libs/win/libusb-1.0.lib
win32: LIBS += -L$$PWD/libs/win -llibusb-1.0

# Runtime files that must sit next to the exe (copied at build time). dfu-programmer is linked
# into the application, so libusb is the only library that has to be deployed.
win32 {
    runtime.files = $$PWD/libs/win/libusb-1.0.dll
    CONFIG(debug, debug|release): runtime.path = $$OUT_PWD/debug
    else: runtime.path = $$OUT_PWD/release
    COPIES += runtime
}

macx: LIBS += $$PWD/libs/mac/libusb-1.0.dylib

unix:!macx: LIBS += -lusb-1.0

macx {
    ICON = "Bitmaps/gt.icns"
}

OTHER_FILES += \
    win-css.qss \
    mac-css.qss \
    mac-tab.qss \
    win-tab.qss \
    customtheme.qss \
    colorDialogStyle.qss

# Licence and attribution files that must ship with every binary (see licenses/README.md)
DISTFILES += \
    gpl.txt \
    licenses/README.md \
    licenses/LGPL-2.1.txt \
    licenses/MIT-QextSerialPort.txt \
    licenses/OFL-1.1.txt \
    third_party/dfu-programmer/COPYING \
    third_party/dfu-programmer/AUTHORS \
    third_party/dfu-programmer/PATCHES.md

include(extserialport/qextserialport.pri)
