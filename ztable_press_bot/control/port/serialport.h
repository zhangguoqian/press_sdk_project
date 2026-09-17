//
// Created by 11518 on 2026/9/12.
//

#ifndef ZTABLE_PRESS_BOT_SERIALPORT_H
#define ZTABLE_PRESS_BOT_SERIALPORT_H

#include <CSerialPort/SerialPort.h>
#include <CSerialPort/SerialPortInfo.h>
#include "portbase.h"

using BaudRate = itas109::BaudRate;
using DataBits = itas109::DataBits;
using Parity = itas109::Parity;
using StopBits = itas109::StopBits;
using FlowControl = itas109::FlowControl;
using SerialPortError = itas109::SerialPortError;

class Serialport : public PortBase
{
public:
    //! @brief 构造函数
    explicit Serialport();
    //! @brief 析构函数
    ~Serialport() override;
    //! @brief 获取串口信息
    //! @return std::vector<itas109::SerialPortInfo> 串口信息向量
    static std::vector<itas109::SerialPortInfo> getSerialPortInfo();
    //! @brief 设置串口名称
    //! @param portName [in] the port name 串口名称
    void setPortName(const char* portName) override;
    //! @brief 打开串口
    //! @return bool 是否打开成功
    bool open() override;
    //! @brief 关闭串口
    void close() override;
    //! @brief 写入数据
    //! @param data [in] the data to write 写入的数据
    //! @param len [in] the length of data the data的长度
    //! @return int 写入的字节数
    int write(const uint8_t* data, int len) override;
    //! @brief 读取数据
    //! @param data [out] the data read from serial port 从串口读取的数据
    //! @param len [in] the length of data the data的长度
    //! @return int 读取的字节数
    int read(uint8_t* data, int len) override;
    //! @brief 判断串口是否打开
    //! @return bool 是否打开
    bool isOpen() override;
private:
    std::shared_ptr<itas109::CSerialPort> mpSerialPort = nullptr;
};


#endif //ZTABLE_PRESS_BOT_SERIALPORT_H
