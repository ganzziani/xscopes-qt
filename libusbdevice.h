#ifndef LIBUSBDEVICE_H
#define LIBUSBDEVICE_H
#include <QObject>
#include <QFuture>
#include <atomic>
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    #include <QtConcurrent/QtConcurrent>
#else
    #include <QtConcurrentRun>
#endif
#include "libusb.h"
#include "libusbdeviceinfo.h"
#include <QDebug>
#include "serialportconnection.h"

typedef uint8_t byte;

class LibUsbDevice : public QObject {
    Q_OBJECT
public:
    explicit LibUsbDevice(QObject *parent = 0);
    void initializeDevice();
    void openDevice(QString = "");
    void closeDevice();
    void stopEventThread();
    void freeTransfers();
    void eventThread();
    void notifyDeviceRemoved();
    bool controlReadTransfer(uint8_t command, uint16_t value = 0, uint16_t index = 0);
    void asyncBulkReadTransfer();
    void awgBulkWriteTransfer();
    void controlWriteTransfer(uint16_t index, uint8_t value);
    QString requestFirmwareVersion();
    void stopScope();
    void startScope();
    void forceTrigger();
    void autoSetup();
    void saveAWG();
    void saveDeviceSettings();
    void restoreSettings();
    void RequestInfo();
    QString getStringFromUnsignedChar(unsigned char *, int);
    void reset();

    int requestMM(unsigned char *buffer);
signals:
    // Emitted from the event thread when the device goes away, connected queued
    void deviceDisconnected();
public slots:
    void newDataAvailable(int);
    void turnOnAutoMode();
    void turnOffAutoMode();
    void clearData();
public:
    bool isDeviceConnected, isInitialiazed;
    QString cstatus;
    libusb_device **devs;
    libusb_device *deviceFound;
    libusb_device *dev;
    libusb_device_descriptor deviceDesc;
    libusb_device_handle *deviceHandle;
    libusb_context *context;
    libusb_hotplug_callback_handle hotplugHandle[2];
    struct libusb_transfer *pcToUsbDeviceTransfer;
    struct libusb_transfer *usbDeviceToPcTransfer;
    bool dataAvailable;
    bool isDeviceRemoved;   // set when libusb reports a lost device
    int count, dataLength;
    uint8_t chData[LEN_BULK_IN_BUFFER];
    byte inBuffer[LEN_CONTROL_BUFFER];
    byte awgBuffer[256];
    std::atomic<bool> enableEventThread;   // read from the event thread
    bool hasHotPlugSupport;
    QFuture<void> future;

    SerialPortConnection serial;
    bool wayOfConnecting;   //0 - usb, 1 - serial port
};

#endif // LIBUSBDEVICE_H
