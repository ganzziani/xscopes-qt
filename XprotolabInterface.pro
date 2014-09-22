#-------------------------------------------------
#
# Project created by QtCreator 2013-10-07T21:22:48
#
#-------------------------------------------------

QT       += core gui serialport concurrent

TARGET = xscope
TEMPLATE = app


SOURCES += main.cpp\
        xprotolabinterface.cpp \
    qcustomplot.cpp \
    libusbdevice.cpp \
    fft.cpp \
    complex.cpp \
    customtheme.cpp \
    serialportconnection.cpp

HEADERS  += xprotolabinterface.h \
    qcustomplot.h \
    libusb.h \
    libusbdevice.h \
    libusbdeviceinfo.h \
    fft.h \
    complex.h \
    sniffer.h \
    customtheme.h \
    serialportconnection.h

FORMS    += xprotolabinterface.ui \
    customtheme.ui

RESOURCES += \
    xprotolabinterface.qrc
RC_FILE = xprotolabinterface.rc

win32: LIBS += -L$$PWD/libs/ -llibusb-1.0

INCLUDEPATH += $$PWD/libs
DEPENDPATH += $$PWD/libs

win32: PRE_TARGETDEPS += $$PWD/libs/libusb-1.0.lib

#greaterThan(QT_MAJOR_VERSION, 4):LIBS += -lQt5Concurrent
greaterThan(QT_MAJOR_VERSION, 4):QT += widgets
greaterThan(QT_MAJOR_VERSION, 4):QT += printsupport

unix:macx: LIBS += $$PWD/libs/libusb-1.0.dylib

OTHER_FILES += \    
    win-css.qss \
    mac-css.qss \
    mac-tab.qss \
    win-tab.qss

macx {
    ICON = "Bitmaps/gt.icns"
}
