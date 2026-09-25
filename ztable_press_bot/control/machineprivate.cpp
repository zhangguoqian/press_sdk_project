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
    std::vector<uint8_t> responseListData;
    bool ret = _commandByteData(GET_ROD_JSON,isCompressed,responseListData);
    if (ret)
    {
        if (0==m_MachineData.jsonToReadOnlyData(responseListData))
        {
            data = m_MachineData.getReadOnlyData();
            return true;
        }
    }
    return false;
}

bool MachinePrivate::getPressData(PressData& data, uint8_t isCompressed)
{
    std::vector<uint8_t> responseListData;
    bool ret = _commandByteData(GET_PD_JSON,isCompressed,responseListData);
    if (ret)
    {
        if (0==m_MachineData.jsonToPressData(responseListData))
        {
            data = m_MachineData.getPressData();
            return true;
        }
    }
    return ret;
}

bool MachinePrivate::getRealTimeData(RealTimeData& data, uint8_t isCompressed)
{
    std::vector<uint8_t> responseListData;
    bool ret = _commandByteData(GET_RT_JSON,isCompressed,responseListData);
    if (ret)
    {
        if (0==m_MachineData.jsonToRealTimeData(responseListData))
        {
            data = m_MachineData.getRealTimeData();
            return true;
        }
    }
    return ret;
}


std::vector<uint8_t> MachinePrivate::_combinedSendData(uint64_t &registerNo,uint16_t &command)
{
    std::vector<uint8_t> sendData;
    registerNo = m_RegisterNo.load();
    command = htons(m_Command.load(std::memory_order_acquire));
    sendData.reserve(CMD_LEN);
    sendData.insert(sendData.end(),
                    reinterpret_cast<uint8_t*>(&registerNo),
                    reinterpret_cast<uint8_t*>(&registerNo) + sizeof(registerNo)); // 注册号:len=8
    sendData.insert(sendData.end(),
                    reinterpret_cast<uint8_t*>(&command),
                    reinterpret_cast<uint8_t*>(&command) + sizeof(command)); // 命令:len=2
    uint16_t dataLen = 0x0001;
    if (command & 0x0010) //设置命令
    {
        dataLen = m_CommandListData.size();
        sendData.reserve(CMD_LEN+dataLen);
        dataLen = htons(dataLen);
        sendData.insert(sendData.end(),
                       reinterpret_cast<uint8_t*>(&dataLen),
                       reinterpret_cast<uint8_t*>(&dataLen) + sizeof(dataLen)); // 命令:len=2
        {
            std::lock_guard<std::mutex> lock(m_SetCommandMutex);
            sendData.insert(sendData.end(), m_CommandListData.begin(), m_CommandListData.end()); // 命令数据设置:len=?
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
        uint8_t getCommandData = m_CommandByteData.load();
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
    }
    else if (readRegisterNo != registerNo)
    {
        return -5;
    }

    uint16_t readCommand = 0;
    memcpy(&readCommand, response.data() + 8, 2);
    if (readCommand != command)
    {
        return -6;
    }
    uint16_t readDataLen = 0;
    memcpy(&readDataLen, response.data() + 10, 2);
    readDataLen = htons(readDataLen);
    if (readCommand & 0x0010) //设置命令响应 (单字节)
    {
        m_ResponseByteData.store(response[12], std::memory_order_relaxed);
        m_ResponseReady.store(true, std::memory_order_release);
        m_GetResponseDataCond.notify_one();
    }
    else //获取命令响应 (列表数据)
    {
        {
            std::lock_guard<std::mutex> lock(m_GetResponseDataMutex);
            m_ResponseListData.assign(response.begin() + 12, response.begin() + 12 + readDataLen);
        }
        m_ResponseReady.store(true, std::memory_order_release);
        m_GetResponseDataCond.notify_one();
    }

    return 0;
}

bool MachinePrivate::_commandByteData(uint16_t command, uint8_t byteData,std::vector<uint8_t> &responseListData)
{
    responseListData.clear();
    m_RunCode.store(-1, std::memory_order_release);
    m_ResponseReady.store(false, std::memory_order_release);
    m_IsHasCommand.store(true, std::memory_order_release);
    m_Command.store(command, std::memory_order_release);
    m_CommandByteData.store(byteData, std::memory_order_release);

    auto now = std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(WAIT_MILLION_SECONDS);
    std::unique_lock<std::mutex> lock(m_GetResponseDataMutex);
    if (!m_GetResponseDataCond.wait_until(lock, now, [&]{ return m_ResponseReady.load(std::memory_order_acquire); }))
    {
        return false;
    }
    responseListData = m_ResponseListData;
    return 0 == m_RunCode.load(std::memory_order_acquire);
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
    int cycle = 10;
    int retLen = 0;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    while (cycle > 0)
    {
        ret = mpPort->read(pData + retLen, 1024);
        if (ret <= 0 && cycle < 8)
        {
            break;
        }
        retLen += ret;
        cycle --;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
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
            if (m_IsHasCommand.load(std::memory_order_acquire))
            {
                m_IsHasCommand.store(false);
                m_RunCode.store(runCommand(),std::memory_order_release);
            }
            else
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
            }
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