#-------------------------------------------------
#
# Project created by QtCreator 2013-10-07T21:22:48
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

TARGET = XprotolabInterface
TEMPLATE = app


SOURCES += main.cpp\
        xprotolabinterface.cpp \
    qcustomplot.cpp \
    libusbdevice.cpp

HEADERS  += xprotolabinterface.h \
    qcustomplot.h \
    libusb.h \
    libusbdevice.h \
    libusbdeviceinfo.h

FORMS    += xprotolabinterface.ui

RESOURCES += \
    xprotolabinterface.qrc

win32: LIBS += -L$$PWD/libs/ -llibusb-1.0

INCLUDEPATH += $$PWD/libs
DEPENDPATH += $$PWD/libs

win32: PRE_TARGETDEPS += $$PWD/libs/libusb-1.0.lib

greaterThan(QT_MAJOR_VERSION, 4):LIBS += -lQt5Concurrent

unix:!macx:!symbian: LIBS += -lusb-1.0
