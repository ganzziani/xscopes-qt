#ifndef SERIALPORTCONNECTION_H
#define SERIALPORTCONNECTION_H

#include <QObject>
#include <QtSerialPort/QSerialPortInfo>
#include <QtSerialPort/QSerialPort>
#include <QDebug>
#include "libusb.h"

#ifdef Q_OS_MAC
    typedef uint8_t byte;
#endif

class SerialPortConnection : public QObject
{
    Q_OBJECT
public:
    explicit SerialPortConnection(QObject *parent = 0);

signals:
    void newData(int);
public slots:
    void connectToPort(QString);
    void close();
    void write(QString);
    void writeByteArray(QByteArray);
    bool bytesAvailable();
    void run();
public:
    QSerialPort *serial;
    bool sendData;
    byte *wsk;
    bool finish;
    int samplingValue;
};

#endif // SERIALPORTCONNECTION_H
