//
// Created by 11518 on 2024/8/14.
//

#ifndef ZTABLE_PRESS_BOT_PROJECT_SERIALPORT_H
#define ZTABLE_PRESS_BOT_PROJECT_SERIALPORT_H

#include <QSerialPort>


class SerialPort : public QSerialPort{
public:
    SerialPort();
    ~SerialPort()override;
    bool initSerialPort(const char *portName);

    void closeCom();

    void delayMs(int ms);

    bool getPressValue(int &value,bool &stop);

    bool setPressValuePlus(int maxValue, int minValue, int overload,int &tryC);

    bool setPressValue(int maxValue, int minValue, int &tryC);

    bool stopPress();

public:
    bool getState(QByteArray &data);

   private:
    bool getByteArrayData(uint16_t cmdID,QByteArray &data);
    QByteArray getSum(QByteArray data);

    QByteArray m_RegisterIdData;
};


#endif //ZTABLE_PRESS_BOT_PROJECT_SERIALPORT_H
