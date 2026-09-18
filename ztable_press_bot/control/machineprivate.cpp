//
// Created by 11518 on 2026/9/13.
//
#include "machineprivate.h"
#include "machine.h"
#include "port/serialport.h"
#include "common/unity.h"
#include <winsock2.h>
#include <iostream>
#include <thread>

constexpr uint64_t ONE_MILLION_SECOND    = 1000;
constexpr uint64_t WAIT_MILLION_SECONDS  = 300;

constexpr int CMD_LEN = 14;


MachinePrivate::MachinePrivate()
{
}

MachinePrivate::~MachinePrivate()
{
    stop();
}

bool MachinePrivate::connect(const char* portName)
{
    if (nullptr == mpPort)
    {
        mpPort = std::make_unique<Serialport>();
    }
    mpPort->setPortName(portName);
    return mpPort->open();
}

void MachinePrivate::disconnect() const
{
    if (nullptr != mpPort)
    {
        mpPort->close();
    }
}

bool MachinePrivate::isConnected() const
{
    if (nullptr == mpPort)
    {
        return false;
    }
    return mpPort->isOpen();
}


uint64_t MachinePrivate::getMachineRegisterNo() const
{
    return m_RegisterNo;
}

bool MachinePrivate::getReadOnlyData(ReadOnlyData& data, uint8_t isCompressed)
{
    m_Command.store(GET_ROD_JSON);
    m_GetCommandData.store(isCompressed);
    auto now = std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(WAIT_MILLION_SECONDS);
    do
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        if (0==m_RunCode.load())
        {
            uint8_t out = m_GetCommandData.load();
            qDebug() << "out:" << out;
            return out == 0;
        }
    }while (now > std::chrono::high_resolution_clock::now());
    return false;
}

bool MachinePrivate::getPressedData(PressData& data, uint8_t isCompressed)
{
    m_Command.store(GET_PD_JSON);
    m_GetCommandData.store(isCompressed);
}

bool MachinePrivate::getRealTimeData(RealTimeData& data, uint8_t isCompressed)
{
    m_Command.store(GET_RT_JSON);
    m_GetCommandData.store(isCompressed);
}


std::vector<uint8_t> MachinePrivate::_combinedSendData(uint64_t &registerNo,uint16_t &command)
{
    std::vector<uint8_t> sendData;
    registerNo = m_RegisterNo.load();
    command = htons(m_Command.load());
    sendData.reserve(CMD_LEN);
    sendData.insert(sendData.end(),
                    reinterpret_cast<uint8_t*>(&registerNo),
                    reinterpret_cast<uint8_t*>(&registerNo) + sizeof(registerNo)); // 注册号:len=8
    sendData.insert(sendData.end(),
                    reinterpret_cast<uint8_t*>(&command),
                    reinterpret_cast<uint8_t*>(&command) + sizeof(command)); // 命令:len=2
    uint16_t dataLen = 0x0001;
    if (command & 0x1000) //设置命令
    {
        dataLen = m_SetCommandData.size();
        sendData.reserve(CMD_LEN+dataLen);
        dataLen = htons(dataLen);
        sendData.insert(sendData.end(),
                       reinterpret_cast<uint8_t*>(&dataLen),
                       reinterpret_cast<uint8_t*>(&dataLen) + sizeof(dataLen)); // 命令:len=2
        {
            std::lock_guard<std::mutex> lock(m_SetCommandMutex);
            sendData.insert(sendData.end(), m_SetCommandData.begin(), m_SetCommandData.end()); // 命令数据设置:len=?
            uint8_t checkSum = Unity::getChecksum(sendData);
            sendData.emplace_back(checkSum); // 校验和:len=1
        }
    }
    else //获取命令
    {
        dataLen = htons(dataLen);
        sendData.insert(sendData.end(),
                        reinterpret_cast<uint8_t*>(&dataLen),
                        reinterpret_cast<uint8_t*>(&dataLen) + sizeof(dataLen)); // 命令:len=2
        uint8_t getCommandData = m_GetCommandData.load();
        sendData.emplace_back(getCommandData); // 命令数据获取:len=1
        uint8_t checkSum = Unity::getChecksum(sendData);
        sendData.emplace_back(checkSum); // 校验和:len=1
    }

    return sendData;
}

int MachinePrivate::_isValidResponseData(std::vector<uint8_t>& response,uint64_t registerNo,uint16_t command)
{
    int len = CMD_LEN;
    if (response.size() < len)
    {
        return -3;
    }
    if (response.back() != Unity::getChecksum({response.begin(), response.end() - 1}))
    {
        return -4;
    }
    uint64_t readRegisterNo = 0;
    memcpy(&readRegisterNo, response.data(), 8);
    if (registerNo == 0)
    {
        m_RegisterNo.store(readRegisterNo);
    }else if (readRegisterNo != registerNo)
    {
        return -5;
    }

    uint16_t readCommand = 0;
    memcpy(&readCommand, response.data() + 8, 2);
    if (readCommand == command)
    {
        return -6;
    }
    uint16_t readDataLen = 0;
    memcpy(&readDataLen, response.data() + 10, 2);
    if (readCommand & 0x1000) //设置命令
    {
        m_SetResponseData.store(response[12],std::memory_order_relaxed);
    }
    else //获取命令
    {
        std::lock_guard<std::mutex> lock(m_GetResponseDataMutex);
        m_GetResponseData = std::vector<uint8_t>(response.begin() + 12, response.begin() + 12 + readDataLen);
    }

    return 0;
}

int MachinePrivate::runCommand()
{
    if (nullptr == mpPort)
    {
        return -1;
    }
    uint64_t registerNo = 0;
    uint16_t command = 0;
    std::vector<uint8_t> sendData = _combinedSendData(registerNo,command);
    int ret = mpPort->write(sendData.data(), sendData.size());
    if (ret != sendData.size())
    {
        return -2;
    }
    uint8_t pData[1024];
    int cycle = 6;
    int retLen = 0;
    do
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        ret = mpPort->read(pData + retLen, 1024);
        if (ret <= 0 && cycle != 6)
        {
            break;
        }
        retLen += ret;
    }
    while (cycle-- > 0);
    std::vector<uint8_t> data = std::vector<uint8_t>(pData, pData + retLen);
    int info = _isValidResponseData(data,registerNo,command);
    if (0 != info)
    {
        return info;
    }
    return 0;
}




void MachinePrivate::run()
{
    m_IsRunning.store(true);
    mpRunThread = std::make_unique<std::thread>([this]
    {
        while (m_IsRunning.load())
        {
            int ret = runCommand();
            m_RunCode.store(ret);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

    });
}

void MachinePrivate::stop()
{
    m_IsRunning.store(false);
    if (mpRunThread && mpRunThread->joinable())
    {
        mpRunThread->join();
    }
    mpRunThread.reset();
}

bool MachinePrivate::isRunning() const
{
    return m_IsRunning.load();
}


std::vector<std::string> MachinePrivate::getPortList()
{
    auto list = Serialport::getSerialPortInfo();
    std::vector<std::string> portList;
    for (auto port : list)
        portList.emplace_back(port.portName);
    return portList;
}

const char* MachinePrivate::getLastErrorInfo()
{
    return m_ErrorInfo;
}

void MachinePrivate::registerDataInterface(MachineDataInterface* dataInterface)
{
    std::lock_guard<std::mutex> lock(m_MachineDataMutex);
    mpMachineDataInterface = dataInterface;
}

void MachinePrivate::unregisterDataInterface()
{
    std::lock_guard<std::mutex> lock(m_MachineDataMutex);
    mpMachineDataInterface = nullptr;
}
