QT  += core gui serialport concurrent

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
    qtooltipslider.cpp

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
    qtooltipslider.h

FORMS += xprotolabinterface.ui \
    customtheme.ui

RESOURCES += \
    xprotolabinterface.qrc

INCLUDEPATH += $$PWD/libs
DEPENDPATH += $$PWD/libs

greaterThan(QT_MAJOR_VERSION, 4):QT += widgets
greaterThan(QT_MAJOR_VERSION, 4):QT += printsupport

win32: PRE_TARGETDEPS += $$PWD/libs/win/libusb-1.0.lib
win32: LIBS += -L$$PWD/libs/win -llibusb-1.0

macx: LIBS += $$PWD/libs/mac/libusb-1.0.dylib

unix: LIBS += -L$$PWD/libs/unix -lusb-1.0

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
