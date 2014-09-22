#include "serialportconnection.h"

SerialPortConnection::SerialPortConnection(QObject *parent) :
    QObject(parent)
{
    serial=NULL;
    sendData=false;
    wsk=NULL;
    finish=true;
    samplingValue=0;
}

void SerialPortConnection::connectToPort(QString name){
    if(serial==NULL || !serial->isOpen()){
        serial=new QSerialPort(name);
        serial->setPortName(name);
        if (serial->open(QIODevice::ReadWrite)) {
            qDebug()<<"SUCCESS";
        }
        serial->setBaudRate(QSerialPort::Baud115200);
        serial->setDataBits(QSerialPort::Data8);
        serial->setParity(QSerialPort::NoParity);
        serial->setStopBits(QSerialPort::OneStop);
        serial->setFlowControl(QSerialPort::NoFlowControl);
        finish=false;
    }
}

void SerialPortConnection::close(){
    if(serial && serial->isOpen()){
        wsk=NULL;
        serial->flush();
        serial->close();
        disconnect(serial);
        delete serial;
        serial=NULL;
    }
}

void SerialPortConnection::write(QString string){
    if(serial && serial->isOpen()){
        serial->write(string.toStdString().c_str());
        while(serial->waitForBytesWritten(1000) );
    }
}
void SerialPortConnection::writeByteArray(QByteArray string){
    if(serial && serial->isOpen()){
        serial->write(string);
        while(serial->waitForBytesWritten(1000) );
    }
}
bool SerialPortConnection::bytesAvailable(){
    if(serial && serial->isOpen())
        return serial->bytesAvailable();
    return false;
}

void SerialPortConnection::run(){
    while(!finish){
        int max_length=769;
        if(this->samplingValue>=11) max_length=770;
        if(!sendData) continue;
        if(!serial || !serial->isOpen()) continue;
        if(serial->bytesAvailable()<max_length) continue;
        if(wsk==NULL) continue;
        char tmp[769];
        int size=serial->read(tmp,max_length);
        qDebug()<<"Size "<<size;
        for(int i=0;i<size;i++){
            wsk[i]=tmp[i];
        }
        if(size==769) wsk[769]=0;
        emit newData(770);
    }
}
