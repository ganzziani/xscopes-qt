QT += core gui serialport concurrent
QT += network

TARGET = xscope
TEMPLATE = app

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
    bootloader.cpp

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
    markerpixmap.h \
    persistenceitem.h

FORMS += xprotolabinterface.ui \
    customtheme.ui \
    bootloader.ui

RESOURCES += \
    xprotolabinterface.qrc

INCLUDEPATH += $$PWD/libs
DEPENDPATH += $$PWD/libs

greaterThan(QT_MAJOR_VERSION, 4):QT += widgets
greaterThan(QT_MAJOR_VERSION, 4):QT += printsupport

win32: PRE_TARGETDEPS += $$PWD/libs/win/libusb-1.0.lib
win32: LIBS += -L$$PWD/libs/win -llibusb-1.0

# Runtime files that must sit next to the exe (copied at build time)
win32 {
    runtime.files = $$PWD/libs/win/libusb-1.0.dll $$PWD/libs/win/dfu-programmer.exe
    CONFIG(debug, debug|release): runtime.path = $$OUT_PWD/debug
    else: runtime.path = $$OUT_PWD/release
    COPIES += runtime
}

macx: LIBS += $$PWD/libs/mac/libusb-1.0.dylib

unix:!macx: LIBS += -lusb-1.0

RC_FILE = xprotolabinterface.rc

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

include(extserialport/qextserialport.pri)
