//
// Created by 11518 on 2026/9/12.
//

#include "serialport.h"


Serialport::Serialport() : PortBase()
{
    mpSerialPort = std::make_shared<itas109::CSerialPort>();

}

Serialport::~Serialport()
{
    mpSerialPort.reset();
}

std::vector<itas109::SerialPortInfo> Serialport::getSerialPortInfo()
{
    return itas109::CSerialPortInfo::availablePortInfos();
}

void Serialport::setPortName(const char* portName)
{
    mpSerialPort->init(portName,BaudRate::BaudRate115200,Parity::ParityNone,DataBits::DataBits8,StopBits::StopOne,FlowControl::FlowNone,1024);
}



bool Serialport::open()
{
    return mpSerialPort->open();
}

void Serialport::close()
{
    mpSerialPort->close();
}

int Serialport::write(const uint8_t* data, int len)
{
    return mpSerialPort->writeData(data,len);
}

int Serialport::read(uint8_t* data, int len)
{
    return mpSerialPort->readData(data,len);
}

bool Serialport::isOpen()
{
    return mpSerialPort->isOpen();
}


