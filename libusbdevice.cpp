#include "libusbdevice.h"
#include "libusbdeviceinfo.h"

#include <QtEndian>
#include <cstring>
#include <QElapsedTimer>
#include <QThread>
#include "qextserialport.h"

LibUsbDevice::LibUsbDevice(QObject *parent) :
    QObject(parent) {
    wayOfConnecting = false;
    enableEventThread = true;
    isDeviceConnected = false;
    isInitialiazed = false;
    deviceHandle = nullptr;
    context = nullptr;
    pcToUsbDeviceTransfer = nullptr;
    usbDeviceToPcTransfer = nullptr;
    dataAvailable = false;
    count = 0;
    dataLength = 0;
    isDeviceRemoved = false;
    hasHotPlugSupport = false;
    deviceFound = nullptr;
    deviceCode = 0;
    descriptorRev = 0;
    streamPaused = false;
    readPending = false;
    latestLength = 0;
    framePending = false;
    clearData();    // chData is read before the device has filled it
    connect(&serial, SIGNAL(newData(int)), this, SLOT(newDataAvailable(int)));
    connect(&serial, SIGNAL(clear()), this, SLOT(clearData()));
}
void LibUsbDevice::reset() {
    enableEventThread = true;
    isDeviceConnected = false;
    isInitialiazed = false;
    deviceHandle = nullptr;
    context = nullptr;
    freeTransfers();
    isDeviceRemoved = false;
    dataAvailable = false;
    hasHotPlugSupport = false;
    deviceFound = nullptr;
    deviceCode = 0;
    descriptorRev = 0;
    QMutexLocker lock(&streamMutex);
    streamPaused = false;
    readPending = false;
    framePending = false;
}

void LibUsbDevice::clearData() {
    for(int i = 0; i < LEN_BULK_IN_BUFFER; i++)
        chData[i] = 0;
    for(int i = 0; i < LEN_CONTROL_BUFFER; i++)
        inBuffer[i] = 0;
    for(int i = 0; i < 256; i++)
        awgBuffer[i] = 0;
}

extern "C" int LIBUSB_CALL hotplugAttachCallback(libusb_context *, libusb_device *, libusb_hotplug_event, void *ref) {
    LibUsbDevice *cthis = static_cast<LibUsbDevice *>(ref);
    cthis->openDevice();
    qDebug() << "Hotplug attach";
    if(cthis->isDeviceConnected)
        cthis->asyncBulkReadTransfer();
    return 0;
}

extern "C" int LIBUSB_CALL hotplugDetachCallback(libusb_context *, libusb_device *, libusb_hotplug_event, void *ref) {
    qDebug() << "Hotplug detach";
    // Runs in the event thread: only flag the loss and let the GUI thread do the
    // cleanup (libusb_close() must not be called from inside a hotplug callback).
    static_cast<LibUsbDevice *>(ref)->notifyDeviceRemoved();
    return 0;
}

void LibUsbDevice::notifyDeviceRemoved() {
    if(isDeviceRemoved)
        return;
    isDeviceRemoved = true;
    // Stop the event loop once the current callback returns, so that
    // closeDevice() is not kept waiting for a device that is gone.
    enableEventThread = false;
    emit deviceDisconnected();
}

extern "C" void LIBUSB_CALL asyncBulkReadTransferCallback(struct libusb_transfer *transfer) {
    LibUsbDevice *cthis = static_cast<LibUsbDevice *>(transfer->user_data);
    if(!cthis->enableEventThread)
        return;     // shutting down, do not resubmit
    if(transfer->status != LIBUSB_TRANSFER_COMPLETED) {
        // LIBUSB_TRANSFER_NO_DEVICE when the device was unplugged. Resubmitting
        // would leave libusb with no pending transfer at all, which is what made
        // the event loop block (and closeDevice() hang) afterwards.
        qDebug() << "Bulk read transfer stopped, status " << transfer->status;
        cthis->notifyDeviceRemoved();
        return;
    }
    QMutexLocker lock(&cthis->streamMutex);
    if(cthis->streamPaused) {
        // Meter mode: drop this frame and stop asking for more (resumeStream())
        cthis->readPending = false;
        return;
    }
    // Publish the frame before its buffer goes back to libusb: the GUI thread takes it
    // from latestFrame (takeFrame()), never from the buffer the next transfer fills.
    std::memcpy(cthis->latestFrame, transfer->buffer, transfer->actual_length);
    cthis->latestLength = transfer->actual_length;
    cthis->framePending = true;
    if(libusb_submit_transfer(transfer) != LIBUSB_SUCCESS) {
        cthis->readPending = false;
        qDebug() << "Cannot resubmit bulk read transfer";
        cthis->notifyDeviceRemoved();
    }
}

extern "C" void LIBUSB_CALL asyncBulkWriteTransferCallback(struct libusb_transfer *) {
}

void LibUsbDevice::initializeDevice() {
    if(wayOfConnecting) {
        if(isInitialiazed)
            return;     // already initialized, do not leak the libusb context
        int status;
        status = libusb_init(&context); //initialize the library for the session we just declared
        if(status != 0) {
            cstatus = tr("Initialization Error!");
            qDebug() << "Initialization Error! " << status << Qt::endl; //there was an error
            isInitialiazed = false;
            return;
        }
        isInitialiazed = true;
        if(libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG) && 0) {
            hasHotPlugSupport = true;
            //status = libusb_hotplug_register_callback (context, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, LIBUSB_HOTPLUG_ENUMERATE , VENDOR_ID,
            //  PRODUCT_ID, CLASS_ID, hotplugAttachCallback, this, &hotplugHandle[0]);
            if(LIBUSB_SUCCESS != status) {
                hasHotPlugSupport = false;
            }
            //status = libusb_hotplug_register_callback (context, LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, LIBUSB_HOTPLUG_ENUMERATE , VENDOR_ID,
            // PRODUCT_ID, CLASS_ID, hotplugDetachCallback, this, &hotplugHandle[1]);
            if(LIBUSB_SUCCESS != status) {
                hasHotPlugSupport = false;
            }
            future = QtConcurrent::run(&LibUsbDevice::eventThread, this);
        }
        libusb_set_option(context, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_INFO);
    } else {
        isInitialiazed = true;
    }
}

void LibUsbDevice::openDevice(QString nameOfPort) {
    if(wayOfConnecting) {
        size_t i = 0;
        isDeviceRemoved = false;
        int status;
        status = libusb_get_device_list(context, &devs);
        if(status < 0) {
            cstatus = tr("Cannot open device!");
            qDebug() << "No USB Devices to List" << Qt::endl;
            return;
        }
        while((dev = devs[i++]) != nullptr) {
            status = libusb_get_device_descriptor(dev, &deviceDesc);
            if(status < 0) {
                break;
            }
            if(deviceDesc.idVendor == VENDOR_ID && deviceDesc.idProduct == PRODUCT_ID) {
                status = libusb_open(dev, &deviceHandle);
                if(status) {
                    cstatus = tr("Cannot open device!");
                    qDebug() << "Cannot open device--" << i << Qt::endl;
                    deviceHandle = NULL;
                } else {
                    qDebug() << dev << " Device Opened" << Qt::endl;
                    break;
                }
            }
        }
        libusb_free_device_list(devs, 1);
        if(deviceHandle == NULL) {
            cstatus = tr("Cannot open device!");
            qDebug() << "Cannot open device" << Qt::endl;
            emit serial.connectionStatus("Cannot open device");
            return;
        }
        if(libusb_kernel_driver_active(deviceHandle, 0) == 1) {
            qDebug() << "Kernel Driver Active" << Qt::endl;
            if(libusb_detach_kernel_driver(deviceHandle, 0) == 0) //detach it
                qDebug() << "Kernel Driver Detached!" << Qt::endl;
        }
        status = libusb_claim_interface(deviceHandle, INTERFACE_NUMBER);
        if(status != LIBUSB_SUCCESS) {
            qDebug() << "Cannot Claim Interface" << Qt::endl;
            libusb_close(deviceHandle);
            isDeviceConnected = false;
            return;
        }
        // Same convention as the firmware's DEVICE_CODE: the low byte names the
        // device, its high nibble the family (sniffer.h reads the payload size
        // from it).
        deviceCode = deviceDesc.bcdDevice & 0xFF;
        descriptorRev = deviceDesc.bcdDevice >> 8;
        isDeviceConnected = true;
    } else {
        isDeviceConnected = serial.connectToPort(nameOfPort);
        if(isDeviceConnected) {
            serial.serial->write("p");
            serial.serial->flush();
        }
    }
}

void LibUsbDevice::freeTransfers() {
    // Must run before libusb_exit(): libusb_free_transfer() unrefs the device the
    // transfer was submitted on, and the context that owns it is gone afterwards.
    if(pcToUsbDeviceTransfer) {
        libusb_free_transfer(pcToUsbDeviceTransfer);
        pcToUsbDeviceTransfer = nullptr;
    }
    if(usbDeviceToPcTransfer) {
        libusb_free_transfer(usbDeviceToPcTransfer);
        usbDeviceToPcTransfer = nullptr;
    }
}

void LibUsbDevice::stopEventThread() {
    // Order matters: the callback checks enableEventThread, so it must be cleared
    // before cancelling, otherwise the cancelled transfer is resubmitted.
    enableEventThread = false;
    if(wayOfConnecting) {
        if(usbDeviceToPcTransfer) {
            libusb_cancel_transfer(usbDeviceToPcTransfer);
        }
        if(pcToUsbDeviceTransfer) {
            libusb_cancel_transfer(pcToUsbDeviceTransfer);
        }
    }
    future.waitForFinished();
}

void LibUsbDevice::closeDevice() {
    if(isDeviceConnected) {
        if(wayOfConnecting) {
            stopEventThread();
            if(hasHotPlugSupport) {
                //            libusb_hotplug_deregister_callback(context, hotplugHandle[0]);
                //            libusb_hotplug_deregister_callback(context, hotplugHandle[1]);
            }
            if(!isDeviceRemoved)     // the handle is dead once the device is unplugged
                saveDeviceSettings();
            freeTransfers();
            if(deviceHandle) {
                libusb_release_interface(deviceHandle, INTERFACE_NUMBER);
                libusb_close(deviceHandle);
                deviceHandle = nullptr;
            }
            if(context) {
                libusb_exit(context);
                context = nullptr;
            }
            isInitialiazed = false;
            isDeviceConnected = false;
            isDeviceRemoved = false;
        } else {
            serial.sendData = false;
            serial.finish = true;
            future.waitForFinished();
            enableEventThread = false;
            serial.write("p");
            saveDeviceSettings();
            serial.close();
            isDeviceConnected = false;
        }
    } else if(isInitialiazed) {
        enableEventThread = false;
        if(wayOfConnecting) {
            if(hasHotPlugSupport) {
                //            libusb_hotplug_deregister_callback(context, hotplugHandle[0]);
                //            libusb_hotplug_deregister_callback(context, hotplugHandle[1]);
            }
            future.waitForFinished();
            freeTransfers();
            if(context) {
                libusb_exit(context);
                context = nullptr;
            }
            isInitialiazed = false;
            isDeviceRemoved = false;
        } else {
            serial.finish = true;
            future.waitForFinished();
            serial.sendData = false;
            serial.write("p");
            serial.close();
        }
    }
}

void LibUsbDevice::eventThread() {
    if(wayOfConnecting) {
        // libusb_handle_events() blocks for up to 60 s when nothing is pending,
        // so a plain blocking call would delay closing the device by that much.
        // A short timeout keeps the shutdown responsive (closeDevice() joins
        // this thread).
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 100 * 1000;
        while(enableEventThread && context) {
            libusb_handle_events_timeout(context, &tv);
        }
    }
    return;
}

bool LibUsbDevice::controlReadTransfer(uint8_t command, uint16_t value, uint16_t index) {
    if(!isDeviceConnected) {
        qDebug() << "Device not connected";
        return false;
    }
    if(wayOfConnecting) {
        int bytesRead;
        bytesRead = libusb_control_transfer(deviceHandle, 0xC0, command, value, index, inBuffer, LEN_CONTROL_BUFFER, 1000);
        if(bytesRead >= 0) {
            if(bytesRead < LEN_CONTROL_BUFFER)
                inBuffer[bytesRead] = '\0';
            return true;
        } else {
            cstatus = tr("Read Error!");
            qDebug() << "read error: " << bytesRead;
            return false;
        }
    } else {
        // Over the UART a command reads only its own parameter bytes (interface.c): 'c' the
        // four bytes of M.AWGdesiredF, as USB's wIndex then wValue, and 'j' the two of
        // M.Tpost (USB's wValue). Anything more runs as the next commands ('j' sent its
        // index first, so Tpost got 0 and the value bytes ran as commands).
        const bool reply = command == 'u';
        turnOffAutoMode();
        if(reply)
            drainSerial();
        QByteArray ba(1, char(command));
        if(command == 'c') {
            ba.append(char(index & 0xFF)).append(char(index >> 8));
            ba.append(char(value & 0xFF)).append(char(value >> 8));
        } else if(command == 'j') {
            ba.append(char(value & 0xFF)).append(char(value >> 8));
        }
        serial.writeByteArray(ba);
        bool ok = true;
        if(reply) {
            ok = serialRead(inBuffer, 44, 1000);
            if(ok)
                inBuffer[44] = '\0';
            else
                cstatus = tr("Read Error!");
        }
        turnOnAutoMode();
        return ok;
    }
    return false;
}

// QextSerialPort has no waitForReadyRead(): poll the port until count bytes are in, or
// the time is up
bool LibUsbDevice::serialRead(uint8_t *buffer, int count, int timeoutMs) {
    if(!serial.serial)
        return false;
    QElapsedTimer elapsed;
    elapsed.start();
    int got = 0;
    while(got < count && elapsed.elapsed() < timeoutMs) {
        const qint64 available = serial.serial->bytesAvailable();
        if(available > 0) {
            const qint64 n = serial.serial->read(reinterpret_cast<char *>(buffer) + got, qMin<qint64>(available, count - got));
            if(n < 0)
                return false;
            got += n;
        } else {
            QThread::msleep(1);
        }
    }
    return got == count;
}

// After 'p' the device can still be sending the rest of a frame: drop bytes until the
// line has been quiet for longer than a USB serial adapter holds them back (16 ms)
void LibUsbDevice::drainSerial() {
    if(!serial.serial)
        return;
    QElapsedTimer total, idle;
    total.start();
    idle.start();
    while(total.elapsed() < 500) {
        if(serial.serial->bytesAvailable() > 0) {
            serial.serial->readAll();
            idle.restart();
        } else if(idle.elapsed() >= 30) {
            break;
        } else {
            QThread::msleep(1);
        }
    }
}

void LibUsbDevice::asyncBulkReadTransfer() {
    if(!isDeviceConnected) {
        qDebug() << "Device not connected";
        return;
    }
    if(wayOfConnecting) {
        if(!usbDeviceToPcTransfer) {
            usbDeviceToPcTransfer = libusb_alloc_transfer(0);
            if(!usbDeviceToPcTransfer) {
                cstatus = tr("Bulk read transfer failed!");
                qDebug() << "Cannot allocate transfer";
                return;
            }
        }
        {
            QMutexLocker lock(&streamMutex);
            if(readPending)
                return;     // one read in flight at a time: the callback resubmits it
            libusb_fill_bulk_transfer(usbDeviceToPcTransfer, deviceHandle, USB_ENDPOINT_IN,
                                      rxBuffer, LEN_BULK_IN_BUFFER, asyncBulkReadTransferCallback, this, 0);
            int status = libusb_submit_transfer(usbDeviceToPcTransfer);
            if(status != 0) {
                cstatus = tr("Bulk read transfer failed!");
                qDebug() << "Transfer Failed! " << status;
                return;
            }
            readPending = true;
        }
        if(!future.isRunning()) {
            enableEventThread = true;
            future = QtConcurrent::run(&LibUsbDevice::eventThread, this);
        }
    } else {
        serial.wsk = chData;
        serial.finish = false;
    }
}

void LibUsbDevice::controlWriteTransfer(uint16_t index, uint8_t value) {
    if(!isDeviceConnected) {
        qDebug() << "Device not connected";
        return;
    }
    if(holdWrites)
        return;
    if(wayOfConnecting)
        libusb_control_transfer(deviceHandle, 0xC0, 'b', value, index, NULL, 0, 1000);
    else {
        QByteArray ba;
        ba[0] = 'b';
        ba[1] = index;
        ba[2] = value;
        serial.writeByteArray(ba);
    }
}

void LibUsbDevice::saveDeviceSettings() {
    if(!isDeviceConnected) {
        qDebug() << "Device not connected";
        return;
    }
    if(wayOfConnecting)
        libusb_control_transfer(deviceHandle, 0xC0, 'd', 0, 0, NULL, 0, 1000);
    else
        serial.write("d");
}

// The custom waveform into the device's AWG buffer: bulk endpoint 2 over USB. Over the
// UART it is the 'x' command: the device answers 'G', reads 256 bytes and answers 'T'.
// (This read 256 bytes from the port instead, which never came.)
bool LibUsbDevice::awgBulkWriteTransfer() {
    if(!isDeviceConnected) {
        qDebug() << "Device not connected";
        return false;
    }
    if(wayOfConnecting) {
        int actual = 0;
        return libusb_bulk_transfer(deviceHandle, USB_ENDPOINT_OUT, awgBuffer, 256, &actual, 1000) == 0 && actual == 256;
    }
    turnOffAutoMode();
    drainSerial();
    serial.write("x");
    uint8_t answer = 0;
    bool ok = serialRead(&answer, 1, 1000) && answer == 'G';
    if(ok) {
        serial.writeByteArray(QByteArray(reinterpret_cast<const char *>(awgBuffer), 256));
        ok = serialRead(&answer, 1, 2000) && answer == 'T';
    }
    if(!ok)
        cstatus = tr("The device did not take the waveform");
    turnOnAutoMode();
    return ok;
}

QString LibUsbDevice::requestFirmwareVersion() {
    if(!isDeviceConnected) {
        qDebug() << "Device not connected";
        return NULL;
    }
    int bytesRead = 0;
    unsigned char buffer[4];
    if(wayOfConnecting) {
        bytesRead = libusb_control_transfer(deviceHandle, 0xC0, 'a', 0, 0, buffer, 4, 1000);
    } else {
        drainSerial();
        serial.write("a");
        if(!serialRead(buffer, 4, 2000)) {
            emit serial.connectionStatus("Port is busy or unreachable.");
            return "-1";
        }
        bytesRead = 4;
    }
    if(bytesRead > 0) {
        return getStringFromUnsignedChar(buffer, 4);
    } else {
        cstatus = tr("Read Error!");
        qDebug() << "read error: " << bytesRead;
        return NULL;
    }
}

// Display screenshot. The device answers the 'C' command with one block of the BMP
// stream per control transfer, wIndex = block number, up to 64 bytes over USB.
// The stream is a 62 byte BMP header followed by the display transposed into 1bpp
// rows, bottom row first: the XScopes send 128x64 (1086 bytes, 17 blocks), the
// Oscilloscope Watch 128x128 (2110 bytes, 33 blocks). The size is read from the
// width and height in the header, so both are handled the same way.
// 'C' reads the display buffer the firmware draws into, 64 bytes at a time (17 blocks,
// 11.6 ms). Where the firmware clears and redraws it every frame - Srate 0..10 and roll mode,
// in the scope, FFT and XY modes - it does so even while stopped, so a read could catch a
// half-drawn frame (half the reads at 128 us/div). Stopped and in persistent mode it no longer
// clears it, so the picture holds still while it is read. At the slower rates the sweep is
// drawn point by point without clearing, plain reads are whole, and persistence would pile
// the new samples onto the picture (they keep coming for a while after the stop), so those
// are read as they are. The device's own settings say whether it is running, which the
// Start/Stop button may not know after a Single capture.
bool LibUsbDevice::requestDisplayBitmap(QByteArray &bmp) {
    bmp.clear();
    if(!isDeviceConnected) {
        qDebug() << "Device not connected";
        return false;
    }
    if(!wayOfConnecting) {
        // Over the serial port 'C' is an XMODEM file transfer, not a block read
        cstatus = tr("Not available on the serial port");
        return false;
    }
    // Registers by GPIO number (main.h in both firmwares): Srate, Mcursors, Display, MFFT, MStatus
    enum { Srate = 0, Mcursors = 6, Display = 7, MFFT = 8, MStatus = 11 };
    enum { roll = 0x01, persistent = 0x20, scopeModes = 0xE0, gosniffer = 0x08, stopped = 0x10 };
    uint8_t settings[LEN_CONTROL_BUFFER];
    const int got = libusb_control_transfer(deviceHandle, 0xC0, 'u', 0, 14, settings,
                                            LEN_CONTROL_BUFFER, 1000);
    const bool freeze = got > MStatus && (settings[MFFT] & scopeModes) &&
            !(settings[MStatus] & gosniffer) && (settings[Srate] <= 10 || (settings[Mcursors] & roll));
    const bool running = freeze && !(settings[MStatus] & stopped);
    if(freeze) {
        if(running) {
            libusb_control_transfer(deviceHandle, 0xC0, 'f', 0, 0, NULL, 0, 1000);
            QThread::msleep(50);    // the frame in progress is still drawn after the stop
        }
        // The change clears the screen once and the stopped frame is drawn again. Without the
        // wait above, the last frame acquired could land on it as a second trace.
        libusb_control_transfer(deviceHandle, 0xC0, 'b', settings[Display] | persistent, Display,
                                NULL, 0, 1000);
        QThread::msleep(100);
    }
    const bool ok = readDisplayBlocks(bmp);
    if(freeze) {
        libusb_control_transfer(deviceHandle, 0xC0, 'b', settings[Display], Display, NULL, 0, 1000);
        if(running)
            libusb_control_transfer(deviceHandle, 0xC0, 'g', 0, 0, NULL, 0, 1000);
    }
    return ok;
}

bool LibUsbDevice::readDisplayBlocks(QByteArray &bmp) {
    uint8_t block[LEN_CONTROL_BUFFER];
    int bmpSize = 0;
    for(int blockIndex = 0; blockIndex < 64; blockIndex++) {
        int bytesRead = libusb_control_transfer(deviceHandle, 0xC0, 'C', 0, blockIndex,
                                                block, LEN_CONTROL_BUFFER, 1000);
        if(bytesRead < 0) {
            cstatus = tr("Read Error!");
            qDebug() << "Display bitmap read error: " << bytesRead;
            bmp.clear();
            return false;
        }
        if(bytesRead == 0)
            break;      // no more blocks (or a firmware without the command)
        if(blockIndex == 0) {
            // 'BM', then the data offset, width and height at offsets 10, 18 and 22
            if(bytesRead < 26 || block[0] != 'B' || block[1] != 'M') {
                cstatus = tr("Device does not send the display bitmap");
                qDebug() << "Unexpected display bitmap header";
                return false;
            }
            int dataOffset = qFromLittleEndian<qint32>(block + 10);   // 62 in both firmwares
            int width = qFromLittleEndian<qint32>(block + 18);
            int height = qFromLittleEndian<qint32>(block + 22);
            if(width <= 0 || height <= 0 || width > 1024 || height > 1024 ||
                    dataOffset < 54 || dataOffset > 4096) {
                cstatus = tr("Invalid display bitmap");
                qDebug() << "Invalid display bitmap size" << width << height;
                return false;
            }
            bmpSize = dataOffset + ((width + 31) / 32 * 4) * height;
        }
        if(bmp.size() + bytesRead > bmpSize)
            bytesRead = bmpSize - bmp.size();
        bmp.append((const char *)block, bytesRead);
        if(bmp.size() >= bmpSize)
            break;
    }
    if(bmpSize == 0) {
        cstatus = tr("Device does not send the display bitmap");
        return false;
    }
    if(bmp.size() != bmpSize) {
        cstatus = tr("Incomplete display bitmap");
        qDebug() << "Incomplete display bitmap" << bmp.size() << "of" << bmpSize;
        bmp.clear();
        return false;
    }
    return true;
}

void LibUsbDevice::stopScope() {
    if(!isDeviceConnected) {
        qDebug() << "Device not connected";
        return;
    }
    if(wayOfConnecting)
        libusb_control_transfer(deviceHandle, 0xC0, 'f', 0, 0, NULL, 0, 1000);
    else
        serial.write("f");
}
void LibUsbDevice::startScope() {
    if(!isDeviceConnected) {
        qDebug() << "Device not connected";
        return;
    }
    if(wayOfConnecting)
        libusb_control_transfer(deviceHandle, 0xC0, 'g', 0, 0, NULL, 0, 1000);
    else
        serial.write("g");
}

void LibUsbDevice::restoreSettings() {
    if(!isDeviceConnected) {
        qDebug() << "Device not connected";
        return;
    }
    if(wayOfConnecting)
        libusb_control_transfer(deviceHandle, 0xC0, 'k', 0, 0, NULL, 0, 1000);
    else
        serial.write("k");
}

void LibUsbDevice::forceTrigger() {
    if(!isDeviceConnected) {
        qDebug() << "Device not connected";
        return;
    }
    if(wayOfConnecting)
        libusb_control_transfer(deviceHandle, 0xC0, 'h', 0, 0, NULL, 0, 1000);
    else
        serial.write("h");
}

void LibUsbDevice::autoSetup() {
    if(!isDeviceConnected) {
        qDebug() << "Device not connected";
        return;
    }
    if(wayOfConnecting)
        libusb_control_transfer(deviceHandle, 0xC0, 'i', 0, 0, NULL, 0, 1000);
    else
        serial.write("i");
}

void LibUsbDevice::saveAWG() {
    if(!isDeviceConnected) {
        qDebug() << "Device not connected";
        return;
    }
    if(wayOfConnecting)
        libusb_control_transfer(deviceHandle, 0xC0, 'e', 0, 0, NULL, 0, 1000);
    else
        serial.write("e");
}

QString LibUsbDevice::getStringFromUnsignedChar(unsigned char *buffer, int length) {
    QString result;
    for(int i = 0; i < length; i++) {
        result.append(QChar(buffer[i]));
    }
    return result;
}
void LibUsbDevice::newDataAvailable(int size) {
    dataAvailable = true;
    dataLength = size;
}

void LibUsbDevice::pauseStream() {
    if(wayOfConnecting) {
        // The read in flight completes with one more frame, which the callback drops.
        // The firmware does not wait for a host that stopped reading (EP1Hold()).
        QMutexLocker lock(&streamMutex);
        streamPaused = true;
        framePending = false;   // a frame from before the pause is not shown after it
    } else {
        turnOffAutoMode();
    }
}

void LibUsbDevice::resumeStream() {
    if(wayOfConnecting) {
        {
            QMutexLocker lock(&streamMutex);
            streamPaused = false;
        }
        if(isDeviceConnected && usbDeviceToPcTransfer)
            asyncBulkReadTransfer();    // no-op while the last read is still pending
    } else {
        turnOnAutoMode();
    }
}

bool LibUsbDevice::takeFrame() {
    if(!wayOfConnecting)
        return false;
    QMutexLocker lock(&streamMutex);
    if(!framePending)
        return false;
    std::memcpy(chData, latestFrame, latestLength);
    dataLength = latestLength;
    framePending = false;
    dataAvailable = true;
    return true;
}

void LibUsbDevice::turnOnAutoMode() {
    if(!wayOfConnecting) {
        serial.write("q");
        serial.sendData = true;
        serial.m_stateOfConnection = 0;
    }
}
void LibUsbDevice::turnOffAutoMode() {
    if(!wayOfConnecting) {
        serial.sendData = false;
        serial.write("p");
    }
}

int LibUsbDevice::requestMM(unsigned char *buffer) {
    if(!isDeviceConnected) {
        qDebug() << "Device not connected";
        return -1;
    }
    int bytesRead = 0;
    if(wayOfConnecting) {
        // Up to 19 bytes: the value, the gains and rates stamped with it, the two
        // ACHANNEL structs and the auto-setup state. Short replies are older
        // firmware; the caller parses what arrived (by length).
        bytesRead = libusb_control_transfer(deviceHandle, 0xC0, 'm', 0, 0, buffer, METER_REPLY_SIZE, 1000);
    } else {
        turnOffAutoMode();
        serial.serial->flush();
        serial.serial->readAll();
        serial.write("m");
        // The UART gets the same reply as USB (the firmware sends all of it), so take
        // all of it: 19 bytes now, 18/14/4 from older firmware. QextSerialPort has no
        // waitForReadyRead(), so poll against deadlines. A reply ends at 19 bytes, or
        // once the value is in and the line has gone quiet for longer than a USB
        // serial adapter holds bytes back (FTDI's latency timer is 16 ms).
        const int kReplyTimeoutMs = 250, kIdleMs = 30;
        QElapsedTimer total, idle;
        total.start();
        idle.start();
        while(bytesRead < METER_REPLY_SIZE && total.elapsed() < kReplyTimeoutMs) {
            const qint64 available = serial.serial->bytesAvailable();
            if(available > 0) {
                const qint64 n = serial.serial->read(reinterpret_cast<char *>(buffer) + bytesRead,
                                                     qMin<qint64>(available, METER_REPLY_SIZE - bytesRead));
                if(n < 0)
                    break;
                bytesRead += n;
                idle.restart();
            } else if(bytesRead >= METER_VALUE_SIZE && idle.elapsed() >= kIdleMs) {
                break;
            } else {
                QThread::msleep(1);
            }
        }
        serial.serial->readAll();   // Nothing past the reply may pass for the next one
    }
    return bytesRead;
}

// USB Bootloader Interface

// Asks the running application to restart into its DFU bootloader (vendor request 0xBB,
// see interface.c in the firmware). The device detaches from USB while it answers, so an
// error or a timeout here is normal: what counts is that it disappears and re-enumerates
// as the Atmel DFU device. Over the serial port the same byte would leave the device in a
// bootloader it cannot be reached through, so this is USB only.
bool LibUsbDevice::requestBootloader() {
    if(!isDeviceConnected || !wayOfConnecting)
        return false;
    libusb_control_transfer(deviceHandle, 0xC0, 0xBB, 0, 0, inBuffer, LEN_CONTROL_BUFFER, 1000);
    return true;
}

void LibUsbDevice::RequestInfo() {
    int bytesRead;
    bytesRead = libusb_control_transfer(deviceHandle, 0xC0, 0xB0, 0, 0, inBuffer, LEN_CONTROL_BUFFER, 1000);
    if(bytesRead >= 0) {
    } else {
        cstatus = tr("Read Error!");
        qDebug() << "read error: " << bytesRead;
    }
}
