//
// Created by 11518 on 2024/8/14.
//

#include "machine.h"
#include <QDebug>
#include <QSerialPortInfo>
#include "pcrhead.h"

#define ARRIVE_SET_VALUE_INTER 0.5 //到达设置值

Machine::Machine(QObject *object) : QThread(object) {
    m_ThreadRunning = false;
    qRegisterMetaType<QSerialPort::SerialPortError>("QSerialPort::SerialPortError");

}

Machine::~Machine() {
    delete mpSerialPort;
}


void Machine::setPortName(const QString &portName) {
    m_PortName = portName;
}


void Machine::run() {
    mpSerialPort = new SerialPort;
    connect(mpSerialPort,SIGNAL(error(QSerialPort::SerialPortError)),this,SIGNAL(signalMachineError(QSerialPort::SerialPortError)));
    mpElapsedTimer = new QElapsedTimer;
    QElapsedTimer elapsedTimer;
    int elapsed = 0;
    int nodeElapsedTime = 0;
    int nodeCycleTime = 1000; //ms
    int nodeInterTime = nodeCycleTime - nodeElapsedTime;
    int sumSize = m_PressCmdList.size();
    int currentIndex = 0;
    bool comInfo = mpSerialPort->initSerialPort(m_PortName.toLocal8Bit().data());
    bool cmdInfo = false;
    int tryCycle = 5;
    int getValue = 0;
    int passTime = 120; //超时
    int timer = 0; //倒计时

    int stopCmdRepeatCount = 3;

    double botSetValueUp = 0.0;
    double botSetValueDown = 0.0;

    double botGetValue = 0.0;

    if(comInfo){
        mpElapsedTimer->start();
        while (m_ThreadRunning){
            elapsedTimer.restart();
            if(currentIndex == sumSize){
                cmdInfo = mpSerialPort->stopPress();
                break;
            }
            tryCycle = 5;
            PressCmd &pressCmd = m_PressCmdList[currentIndex];

            if (pressCmd.s_CmdType == CmdTypeSet){  //设置
                botSetValueUp = APP::tonToMachineKpa(pressCmd.s_UpValue);
                botSetValueDown = APP::tonToMachineKpa(pressCmd.s_DownValue);
                cmdInfo = mpSerialPort->setPressValue(botSetValueUp,
                                                   botSetValueDown,
                                                   tryCycle);
                timer = pressCmd.s_Timer;
//                qDebug() << pressCmd.s_CmdType << botSetValueUp << botSetValueDown;

            }else if(pressCmd.s_CmdType == CmdTypeGet){ //获取
                bool isStop = false;
                cmdInfo = mpSerialPort->getPressValue(getValue,isStop);
                botGetValue = APP::machineKpaToTon(getValue);
                if(botGetValue >= pressCmd.s_UpValue-ARRIVE_SET_VALUE_INTER || passTime == 0 || timer==0){
                    passTime = 0;
                    timer --;
                }else{
                    currentIndex--;
                    passTime --;
                }
                emit signalTimeTimerPress(pressCmd.s_Step,pressCmd.s_UpValue,elapsed,timer,botGetValue,getValue);
//                qDebug() << pressCmd.s_CmdType << botGetValue;
                if(isStop){
                    break;
                }
            }else if(pressCmd.s_CmdType == CmdTypeStop){ //停止
                cmdInfo = mpSerialPort->stopPress();
            }

            elapsed = mpElapsedTimer->elapsed();

            currentIndex++;
            nodeElapsedTime = (int)elapsedTimer.elapsed();
            nodeInterTime = nodeCycleTime - nodeElapsedTime;
            mpSerialPort->delayMs(nodeInterTime);
            if(!cmdInfo){
                emit signalErrorInfo(2);
                break;
            }
        }

        while(stopCmdRepeatCount--){
            mpSerialPort->delayMs(nodeCycleTime);
            comInfo = mpSerialPort->stopPress();
            if(!cmdInfo){
                emit signalErrorInfo(2);
            }
        }
    }else{
        emit signalErrorInfo(1);
    }


    delete mpElapsedTimer;
    mpSerialPort->closeCom();
    delete mpSerialPort;
    emit signalComplete();
}

void Machine::setPressCmdList(const QVector<PressCmd> &pressCmdList) {
    m_PressCmdList = pressCmdList;
}

void Machine::stopMachine() {
    m_ThreadRunning = false;

}

void Machine::startMachine() {
    m_ThreadRunning = true;
    this->start(QThread::HighPriority);
}

bool Machine::machineIsRunning() const {
    return m_ThreadRunning;
}

void Machine::slotGetComPortList() {
    QString portName;
    SerialPort serialPort;
//    serialPort.setBaudRate(QSerialPort::Baud115200);
//    serialPort.setParity(QSerialPort::NoParity);
//    serialPort.setDataBits(QSerialPort::Data8);
//    serialPort.setStopBits(QSerialPort::OneStop);
//    serialPort.setFlowControl(QSerialPort::NoFlowControl);
//    serialPort.setReadBufferSize(256);
    const QList<QSerialPortInfo> serialPortList = QSerialPortInfo::availablePorts();

    for (auto &var:serialPortList){
        // qDebug() << var.portName();
        if(!var.isBusy()){
            if(serialPort.initSerialPort(var.portName().toLocal8Bit().data())){
                if(serialPort.stopPress()){
                    portName = var.portName();
                    break;
                }
            }
            serialPort.closeCom();
        }
    }
    emit signalPortName(portName);
}

