//
// Created by 11518 on 2024/8/14.
//

#ifndef ZTABLE_PRESS_BOT_PROJECT_MACHINE_H
#define ZTABLE_PRESS_BOT_PROJECT_MACHINE_H

#include <QThread>
#include <QElapsedTimer>
#include "serialport.h"
#include "data/actualpressdata.h"

class Machine : public QThread{
Q_OBJECT
signals:
    void signalMachineError(QSerialPort::SerialPortError);
    void signalTimeTimerPress(int,double,int,int,double,int); //阶段值，设定值，时间，计时器，压力,油压强
    void signalPortName(QString);
    void signalComplete();
    void signalErrorInfo(int);
public:
    explicit Machine(QObject *object = nullptr);

    ~Machine() override;

    void setPortName(const QString &portName);
    void setPressCmdList(const QVector<PressCmd> &pressCmdList);
    void startMachine();
    void startPust(); //开始脱模
    void stopMachine();

    bool machineIsRunning() const;

protected:
    void run() override;
    QString m_PortName;
    SerialPort *mpSerialPort;
    QElapsedTimer *mpElapsedTimer;
    bool m_ThreadRunning;
    bool m_Pushing;
    QVector<PressCmd> m_PressCmdList;

protected slots:
    void slotGetComPortList();
};


#endif //ZTABLE_PRESS_BOT_PROJECT_MACHINE_H
