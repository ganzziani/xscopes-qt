#-------------------------------------------------
#
# Project created by QtCreator 2013-10-07T21:22:48
#
#-------------------------------------------------

QT       += core gui



TARGET = xscope
TEMPLATE = app


SOURCES += main.cpp\
        xprotolabinterface.cpp \
    qcustomplot.cpp \
    libusbdevice.cpp \
    fft.cpp \
    complex.cpp \
    customtheme.cpp

HEADERS  += xprotolabinterface.h \
    qcustomplot.h \
    libusb.h \
    libusbdevice.h \
    libusbdeviceinfo.h \
    fft.h \
    complex.h \
    sniffer.h \
    customtheme.h

FORMS    += xprotolabinterface.ui \
    customtheme.ui

RESOURCES += \
    xprotolabinterface.qrc
RC_FILE = xprotolabinterface.rc

win32: LIBS += -L$$PWD/libs/ -llibusb-1.0

INCLUDEPATH += $$PWD/libs
DEPENDPATH += $$PWD/libs

win32: PRE_TARGETDEPS += $$PWD/libs/libusb-1.0.dll.a

greaterThan(QT_MAJOR_VERSION, 4):LIBS += -lQt5Concurrent

unix:!macx:!symbian: LIBS += -lusb-1.0

OTHER_FILES +=
