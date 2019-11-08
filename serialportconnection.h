#ifndef SERIALPORTCONNECTION_H
#define SERIALPORTCONNECTION_H

#include <QObject>
#include <QtSerialPort/QSerialPortInfo>
#include <QtSerialPort/QSerialPort>
#include <QDebug>
#include "libusb.h"
#include <QTimer>

#ifdef Q_OS_MAC
    typedef uint8_t byte;
#endif

#ifdef Q_OS_UNIX
    typedef uint8_t byte;
#endif

class QextSerialPort;

class SerialPortConnection : public QObject {
    Q_OBJECT
public:
    explicit SerialPortConnection(QObject *parent = 0);

signals:
    void newData(int);
    void clear();
    void connectionStatus(QString status);
public slots:
    bool connectToPort(QString);
    void close();
    void write(QString);
    void writeByteArray(QByteArray);
    bool bytesAvailable();
    void setSamplingValue(int value);

    void clearPort();
    bool checkIfEndOfFrame(char tab[]);
    bool checkIfStartOfFrame(char tab[], bool mode);

    void onReadyRead();
public:
    //QSerialPort *serial;
    QextSerialPort *serial;
    bool sendData;
    byte *wsk;
    bool finish;
    int samplingValue;
    int index;

    int m_stateOfConnection;    //0 - wait for start of frame, 1 - get data, 2 - wait for end of frame

    int end_of_frame[3];
    int start_of_frame_fast[3];
    int start_of_frame_slow[3];

    QTimer timer;
};

#endif // SERIALPORTCONNECTION_H
