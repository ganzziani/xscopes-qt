#ifndef LIBUSBDEVICE_H
#define LIBUSBDEVICE_H
#include <QObject>
#include <QByteArray>
#include <QFuture>
#include <QMutex>
#include <atomic>
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    #include <QtConcurrent/QtConcurrent>
#else
    #include <QtConcurrentRun>
#endif
#include "libusb.h"
#include "libusbdeviceinfo.h"
#include "sniffer.h"
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
    bool awgBulkWriteTransfer();
    bool serialRead(uint8_t *buffer, int count, int timeoutMs);
    void drainSerial();
    void controlWriteTransfer(uint16_t index, uint8_t value);
    QString requestFirmwareVersion();
    // The device's display as a BMP, over USB. At the rates where the firmware redraws every
    // frame it holds the picture still while reading (stop + persistent display, ~170 ms).
    bool requestDisplayBitmap(QByteArray &bmp);
    bool readDisplayBlocks(QByteArray &bmp);    // the 'C' block reads alone
    void stopScope();
    void startScope();
    void forceTrigger();
    void autoSetup();
    void saveAWG();
    void saveDeviceSettings();
    void restoreSettings();
    void RequestInfo();
    bool requestBootloader();
    QString getStringFromUnsignedChar(unsigned char *, int);
    void reset();

    int requestMM(unsigned char *buffer);
    // Stop and restart the scope stream (Meter mode needs only 'm'). Over USB the
    // bulk read is not resubmitted while paused; over serial it is auto mode.
    void pauseStream();
    void resumeStream();
    // GUI thread: move the latest whole USB transfer into chData/dataLength and set
    // dataAvailable. False when none arrived since the last call (or on serial, which
    // fills chData itself on the GUI thread).
    bool takeFrame();
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
    // While set, controlWriteTransfer() sends nothing: the app is loading the device's
    // own settings into its controls (XprotolabInterface::readDeviceSettings()).
    bool holdWrites = false;
    // bcdDevice low byte: DEVICE_CODE in the firmware's hardware.h, 0 when the
    // serial port is in use or the descriptor was not read. The sniffer stream's
    // geometry differs between the families (sniffer.h).
    int deviceCode;
    // bcdDevice high byte (DESCRIPTOR_REV in the firmware's Descriptors.c), 0 like deviceCode.
    // 2 and up: the XScopes firmware starts its DFU bootloader on requestBootloader().
    int descriptorRev;
    // Whether requestBootloader() may be sent. The XScopes' 0xBB froze the device off USB
    // until a power cycle before descriptor revision 2; the Watch (0x20) always handled it.
    // Released firmware of both families reports device code 0 and cannot be told apart, so
    // it still gets the request (an old XScopes needs K1 and a power cycle either way).
    bool canRequestBootloader() const {
        return deviceCode == 0 || deviceCode == 0x20 || descriptorRev >= 2;
    }
    int snifferPayloadSize() const {
        return snifferPayloadSizeForDeviceCode(deviceCode);
    }
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
    // dataAvailable, dataLength and chData: GUI thread only. Over USB, takeFrame() copies
    // a finished transfer in; libusb never writes here, so a frame cannot change while
    // plotData() reads it.
    bool dataAvailable;
    bool isDeviceRemoved;   // set when libusb reports a lost device
    int count, dataLength;
    uint8_t chData[LEN_BULK_IN_BUFFER];
    // Event thread: the bulk IN transfer's buffer, and the last whole transfer the
    // callback published from it (under streamMutex, with latestLength/framePending).
    uint8_t rxBuffer[LEN_BULK_IN_BUFFER];
    uint8_t latestFrame[LEN_BULK_IN_BUFFER];
    int latestLength;
    bool framePending;
    byte inBuffer[LEN_CONTROL_BUFFER];
    byte awgBuffer[256];
    std::atomic<bool> enableEventThread;   // read from the event thread
    // The bulk read and pauseStream()/resumeStream() decide under this lock whether
    // a read is in flight, so a resume cannot race the callback that stops resubmitting.
    // It also guards latestFrame/latestLength/framePending.
    QMutex streamMutex;
    bool streamPaused;
    bool readPending;
    bool hasHotPlugSupport;
    QFuture<void> future;

    SerialPortConnection serial;
    bool wayOfConnecting;   // true: USB, false: serial port
};

#endif // LIBUSBDEVICE_H
