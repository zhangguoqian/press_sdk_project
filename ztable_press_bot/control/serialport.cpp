//
// Created by 11518 on 2024/8/14.
//

#include "serialport.h"
#include <QDebug>
#include <QEventLoop>
#include <QTimer>

SerialPort::SerialPort() {

}

SerialPort::~SerialPort() {

}

inline void InvertUint8(unsigned char *dBuf, unsigned char *srcBuf) {
    int i;
    unsigned char tmp[4] = {0};
    for (i = 0; i < 8; i++) {
        if (srcBuf[0] & (1 << i))
            tmp[0] |= 1 << (7 - i);
    }
    dBuf[0] = tmp[0];
}

inline void InvertUint16(unsigned short *dBuf, unsigned short *srcBuf) {
    int i;
    unsigned short tmp[4] = {0};
    for (i = 0; i < 16; i++) {
        if (srcBuf[0] & (1 << i))
            tmp[0] |= 1 << (15 - i);
    }
    dBuf[0] = tmp[0];
}

//crc16/ibm校验
inline unsigned short CRC16_IBM(unsigned char *data, unsigned int datalen) {
    unsigned short wCRCin = 0x0000;
    unsigned short wCPoly = 0x8005;
    unsigned char wChar = 0;
    while (datalen--) {
        wChar = *(data++);
        InvertUint8(&wChar, &wChar);
        wCRCin ^= (wChar << 8);
        for (int i = 0; i < 8; i++) {
            if (wCRCin & 0x8000){
                wCRCin = (unsigned short)((wCRCin << 1) ^ wCPoly);
            }else{
                wCRCin = (unsigned short)(wCRCin << 1);
            }
        }
    }
    InvertUint16(&wCRCin, &wCRCin);
    return (wCRCin);
}

bool SerialPort::initSerialPort(const char *portName) {
//    int baudRate = itas109::BaudRate9600,
//    itas109::Parity parity = itas109::ParityNone,
//    itas109::DataBits dataBits = itas109::DataBits8,
//    itas109::StopBits stopbits = itas109::StopOne,
//    itas109::FlowControl flowControl = itas109::FlowNone,

    this->setPortName(portName);
    this->setBaudRate(QSerialPort::Baud115200);
    this->setParity(QSerialPort::NoParity);
    this->setDataBits(QSerialPort::Data8);
    this->setStopBits(QSerialPort::OneStop);
    this->setFlowControl(QSerialPort::NoFlowControl);
    this->setReadBufferSize(256);
//    if(this->isOpen()){
//        return true;
//    }
    return this->open(QIODevice::ReadWrite);
}


bool SerialPort::setPressValuePlus(int maxValue, int minValue, int overload,int &tryC) {
    const int size = 12;
    char bytes[size] ={char(0xAA),
                       0x55,
                       0x05,
                       char(maxValue>>8),
                       char(maxValue),
                       char(minValue>>8),
                       char(minValue),
                       0x01,
                       char(overload>>8),
                       char(overload),
                       0x0D,
                       0x0A};
    tryC--;
    for(int i(2);i<size;i++){
        if(bytes[i]==char(0xAA)){
            bytes[i] = char(0xAB);
        }
    }
    this->write(bytes, size);
    int cycle = 5;
    int recSize = 0;
    QByteArray byteArray;
    do {
        delayMs(50);
        byteArray += this->readAll();
        recSize = byteArray.size();
    } while (recSize < 10 && cycle --);
    if(recSize>=10 && byteArray[0]==char(0XED)&&byteArray[1]==char(0XDE)&&byteArray[2]==char(0X05))
    {
        return true;
    }
    if(tryC){
        return setPressValuePlus(maxValue, minValue,overload,tryC);
    }
    return false;
}

bool SerialPort::getPressValue(int &value,bool &stop) {
    //AA 55 02 Num2 00 00 00 00 0D 0A
    char bytes[10];
    bytes[0] = char(0xAA);
    bytes[1] = 0x55;
    bytes[2] = 0x02;
    bytes[3] = 0x00;
    bytes[4] = 0x00;
    bytes[5] = 0x00;
    bytes[6] = 0x00;
    bytes[7] = 0x00;
    bytes[8] = 0x0D;
    bytes[9] = 0x0A;
    //ED DE 02 Num2 PressH PressL 00 00 CRCH CRCL
    this->write(bytes,10);
    int cycle = 5;
    int recSize = 0;
    QByteArray byteArray;
    do {
        delayMs(50);
        byteArray += this->readAll();
        recSize = byteArray.size();
    } while (recSize < 10 && cycle --);
    if(recSize>=10 && byteArray[0]==char(0XED)&&byteArray[1]==char(0XDE)&&byteArray[2]==char(0X02))
    {
        value = int(byteArray[5])+(int(byteArray[4])<<8);
        int valueCrc = CRC16_IBM((uchar*)byteArray.data(),8);
        if(byteArray[3] != char(0)){
            stop = true;
        }
        return true;
    }

    return false;
}

bool SerialPort::stopPress() {
    const int size = 10;
    char bytes[size] ={char(0xAA),0x55,0x01,00,0,0,0,0x03,0xD,0xA};
    this->write(bytes, size);
    int cycle = 5;
    int recSize = 0;
    QByteArray byteArray;
    do {
        delayMs(50);
        byteArray += this->readAll();
        recSize = byteArray.size();
    } while (recSize < 10 && cycle --);
    if(recSize>=10 && byteArray[0]==char(0XED)&&byteArray[1]==char(0XDE)&&byteArray[2]==char(0X01))
    {
        return true;
    }
    return false;
}


void SerialPort::closeCom() {
    if(this->isOpen()){
        this->close();
    }
}

void SerialPort::delayMs(int ms) {
    if(ms > 1){
        QEventLoop loop;//定义一个新的事件循环
        QTimer::singleShot(ms, &loop, SLOT(quit()));//创建单次定时器，槽函数为事件循环的退出函数
        loop.exec();//事件循环开始执行，程序会卡在这里，直到定时时间到，本循环被退出
    }
}

bool SerialPort::setPressValue(int maxValue, int minValue, int &tryC) {
//    {0xAA,0x55,0x01,maxValue>>8,maxValue,minValue>>8,minValue,0x01,0xD,0xA};
    const int size = 10;
    char bytes[size] ={char(0xAA),
                       0x55,
                       0x01,
                       char(maxValue>>8),
                       char(maxValue),
                       char(minValue>>8),
                       char(minValue),
                       0x01,
                       0x0D,
                       0x0A};
    tryC--;
    for(int i(2);i<size;i++){
        if(bytes[i]==char(0xAA)){
            bytes[i] = char(0xAB);
        }
    }
    this->write(bytes, size);
    int cycle = 5;
    int recSize = 0;
    QByteArray byteArray;
    do {
        delayMs(50);
        byteArray += this->readAll();
        recSize = byteArray.size();
    } while (recSize < 10 && cycle --);
    if(recSize>=10 && byteArray[0]==char(0XED)&&byteArray[1]==char(0XDE)&&byteArray[2]==char(0X01))
    {
        tryC = 5;
        return true;
    }
    if(tryC){
        return setPressValue(maxValue, minValue,tryC);
    }
    return false;
}
