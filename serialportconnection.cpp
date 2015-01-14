#include "serialportconnection.h"
#include <QThread>
#include "qextserialport.h"

SerialPortConnection::SerialPortConnection(QObject *parent) :
    QObject(parent)
{
    serial=NULL;
    sendData=false;
    wsk=NULL;
    finish=true;
    samplingValue=0;
    index = 0;

    m_stateOfConnection = 1;

    end_of_frame[0] = 10;
    end_of_frame[1] = 13;
    end_of_frame[2] = 55;

    start_of_frame_fast[0] = 13;
    start_of_frame_fast[1] = 10;
    start_of_frame_fast[2] = 70;

    start_of_frame_slow[0] = 13;
    start_of_frame_slow[1] = 10;
    start_of_frame_slow[2] = 83;

    thread()->setPriority(QThread::HighestPriority);
    connect(&timer,SIGNAL(timeout()),this,SLOT(onReadyRead()));
}

bool SerialPortConnection::connectToPort(QString name){
    if(serial==NULL || (serial!=NULL && !serial->isOpen())){

        PortSettings settings = {BAUD115200, DATA_8, PAR_NONE, STOP_1, FLOW_OFF, 10};
        serial = new QextSerialPort(name, settings, QextSerialPort::Polling);        
        serial->setPortName(name);

        if (serial->open(QIODevice::ReadWrite)) {
            qDebug()<<"SUCCESS";
            emit clear();
            timer.start(10);
            finish=false;
            return true;
        }else{
            clearPort();
            return false;
        }
    }
    return false;
}

void SerialPortConnection::clearPort(){
    wsk = NULL;
    if(serial){
        delete serial;
        serial = NULL;
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
        timer.stop();
    }
}

void SerialPortConnection::write(QString string){
    if(serial && serial->isOpen()){
        serial->write(string.toLatin1());
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

void SerialPortConnection::setSamplingValue(int value){
    if(samplingValue < 11 && value >= 11){
        index = 0;
        if(serial && serial->isOpen()){
            serial->flush();
        }
        m_stateOfConnection = 0;
        emit clear();
    }
    if(samplingValue >= 11 && value < 11){
        m_stateOfConnection = 0;
        if(serial && serial->isOpen())
            serial->flush();
    }
    this->samplingValue = value;    
}

void SerialPortConnection::onReadyRead(){
    int max_length = 768;
    if(!sendData) return;
    if(!serial || !serial->isOpen()) return;
    if(!serial->bytesAvailable()) return;
    if(wsk == NULL) return;
    char tmp[769];

    if(samplingValue >= 11){
        int max = 3*(serial->bytesAvailable()/3);
        if(max == 0) return;
        if(max > 768) max = 768;
        int size = serial->read(tmp,max);
        for(int i = 0; i < size; i += 3){
            if(checkIfStartOfFrame(tmp + i, false)){
                index = 0;
                m_stateOfConnection = 1;
                continue;
            }
            if(checkIfEndOfFrame(tmp + i)){
                index = 0;
                continue;
            }
            if(m_stateOfConnection == 0) continue;
            wsk[index ] = tmp[i];
            wsk[index + 256] = tmp[i+1];
            wsk[index + 512] = tmp[i+2];
            index ++;
            emit newData(3);
        }
    }else{
        if(m_stateOfConnection == 0){
            bool end = false;
            while(!end){                
                if(serial->bytesAvailable() < 3){
                    return;
                }
                char tmp_start_of_frame[3];
                serial->read(tmp_start_of_frame,3);

                if(checkIfStartOfFrame(tmp_start_of_frame,true)){
                    m_stateOfConnection = 1;
                    end = true;
                }
            }

        }else if(m_stateOfConnection == 1){
            if(serial->bytesAvailable() < 768){
                return;
            }
            int size = serial->read(tmp, max_length);
            for(int i = 0; i < size; i++){
                wsk[i] = tmp[i];
            }
            if(size == 768) {
                wsk[768] = 0;
                wsk[769] = 0;
            }
            emit newData(770);
            m_stateOfConnection = 2;
        }else if(m_stateOfConnection == 2){
            if(serial->bytesAvailable() < 3){
                return;
            }
            char tmp_end_of_frame[3];
            serial->read(tmp_end_of_frame,3);
            if(checkIfEndOfFrame(tmp_end_of_frame))
                m_stateOfConnection = 0;
        }
    }
}

bool SerialPortConnection::checkIfEndOfFrame(char tab[]){
    return ((int)tab[0] == end_of_frame[0] && (int)tab[1] == end_of_frame[1] && (int)tab[2] == end_of_frame[2]);
}

bool SerialPortConnection::checkIfStartOfFrame(char tab[], bool mode){
    if(mode)
        return ((int)tab[0] == start_of_frame_fast[0] && (int)tab[1] == start_of_frame_fast[1] && (int)tab[2] == start_of_frame_fast[2]);
    else
        return ((int)tab[0] == start_of_frame_slow[0] && (int)tab[1] == start_of_frame_slow[1] && (int)tab[2] == start_of_frame_slow[2]);
}


