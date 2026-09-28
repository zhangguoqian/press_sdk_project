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
    bool ret = mpPort->open();
    if (!ret)
    {
        _setError(MachineError::ConnectFailed);
    }
    return ret;
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
    if (!_requestCommand(GET_ROD_JSON, isCompressed, responseListData))
    {
        if (auto* iface = _safeGetInterface())
        {
            iface->onReadOnlyData(static_cast<int>(m_LastError.load()), m_RegisterNo.load(), ReadOnlyData{});
        }
        return false;
    }
    if (0 != m_MachineData.jsonToReadOnlyData(responseListData))
    {
        _setError(MachineError::JsonParseFailed);
        if (auto* iface = _safeGetInterface())
        {
            iface->onReadOnlyData(static_cast<int>(MachineError::JsonParseFailed), m_RegisterNo.load(), ReadOnlyData{});
        }
        return false;
    }
    data = m_MachineData.getReadOnlyData();
    if (auto* iface = _safeGetInterface())
    {
        iface->onReadOnlyData(0, m_RegisterNo.load(), data);
    }
    return true;
}

bool MachinePrivate::getPressData(PressData& data, uint8_t isCompressed)
{
    std::vector<uint8_t> responseListData;
    if (!_requestCommand(GET_PD_JSON, isCompressed, responseListData))
    {
        if (auto* iface = _safeGetInterface())
        {
            iface->onPressData(static_cast<int>(m_LastError.load()), PressData{});
        }
        return false;
    }
    if (0 != m_MachineData.jsonToPressData(responseListData))
    {
        _setError(MachineError::JsonParseFailed);
        if (auto* iface = _safeGetInterface())
        {
            iface->onPressData(static_cast<int>(MachineError::JsonParseFailed), PressData{});
        }
        return false;
    }
    data = m_MachineData.getPressData();
    if (auto* iface = _safeGetInterface())
    {
        iface->onPressData(0, data);
    }
    return true;
}

bool MachinePrivate::setPressData(const PressData& data, uint8_t isCompressed)
{
    static constexpr uint16_t CMD = SET_PD_JSON;
    std::vector<uint8_t> responseListData;
    std::vector<uint8_t> commandListData;
    commandListData.emplace_back(isCompressed);
    std::vector<uint8_t> tempDataList = data.toFrameData();
    commandListData.insert(commandListData.end(), tempDataList.begin(), tempDataList.end());
    if (!_requestCommand(CMD, commandListData, responseListData))
    {
        if (auto* iface = _safeGetInterface()) iface->onError(CMD, responseListData);
        return false;
    }
    if (responseListData.size() == 1)
    {
        if (responseListData[0] == 0x00)
        {
            return true;
        }
        else if (responseListData[0] == 0x01)
        {
            _setError(MachineError::OutSignalBlocked);
            if (auto* iface = _safeGetInterface()) iface->onError(CMD, responseListData);
            return false;
        }
        else if (responseListData[0] == 0x02)
        {
            _setError(MachineError::ImmediateSignalBlocked);
            return true;
        }
        else if (responseListData[0] == 0x03)
        {
            _setError(MachineError::StateUnchanged);
            if (auto* iface = _safeGetInterface()) iface->onError(CMD, responseListData);
            return false;
        }
        else
        {
            _setError(MachineError::UnknownError);
            if (auto* iface = _safeGetInterface()) iface->onError(CMD, responseListData);
            return false;
        }
    }
    if (auto* iface = _safeGetInterface()) iface->onError(CMD, responseListData);
    return false;
}

bool MachinePrivate::getRealTimeData(RealTimeData& data, uint8_t isCompressed)
{
    std::vector<uint8_t> responseListData;
    if (!_requestCommand(GET_RT_JSON, isCompressed, responseListData))
    {
        if (auto* iface = _safeGetInterface())
        {
            iface->onRealTimeData(static_cast<int>(m_LastError.load()), RealTimeData{});
        }
        return false;
    }
    if (0 != m_MachineData.jsonToRealTimeData(responseListData))
    {
        _setError(MachineError::JsonParseFailed);
        if (auto* iface = _safeGetInterface())
        {
            iface->onRealTimeData(static_cast<int>(MachineError::JsonParseFailed), RealTimeData{});
        }
        return false;
    }
    data = m_MachineData.getRealTimeData();
    if (auto* iface = _safeGetInterface())
    {
        iface->onRealTimeData(0, data);
    }
    return true;
}

bool MachinePrivate::setPressing(uint8_t isPressing)
{
    static constexpr uint16_t CMD = SET_PRESS;
    std::vector<uint8_t> responseListData;
    if (!_requestCommand(CMD, isPressing, responseListData))
    {
        if (auto* iface = _safeGetInterface()) iface->onError(CMD, responseListData);
        return false;
    }
    if (responseListData.size() == 1)
    {
        if (responseListData[0] == 0x00) return true;
        if (responseListData[0] == 0x01)
        {
            _setError(MachineError::OutSignalBlocked);
            if (auto* iface = _safeGetInterface()) iface->onError(CMD, responseListData);
            return false;
        }
        if (responseListData[0] == 0x02)
        {
            _setError(MachineError::ImmediateSignalBlocked);
            return true;
        }
        if (responseListData[0] == 0x03)
        {
            _setError(MachineError::StateUnchanged);
            if (auto* iface = _safeGetInterface()) iface->onError(CMD, responseListData);
            return false;
        }
        _setError(MachineError::UnknownError);
        if (auto* iface = _safeGetInterface()) iface->onError(CMD, responseListData);
        return false;
    }
    if (auto* iface = _safeGetInterface()) iface->onError(CMD, responseListData);
    return false;
}

bool MachinePrivate::setDemolding(uint8_t isDemolding)
{
    static constexpr uint16_t CMD = SET_DEMOLD;
    std::vector<uint8_t> responseListData;
    if (!_requestCommand(CMD, isDemolding, responseListData))
    {
        if (auto* iface = _safeGetInterface()) iface->onError(CMD, responseListData);
        return false;
    }
    if (responseListData.size() == 1)
    {
        if (responseListData[0] == 0x00) return true;
        if (responseListData[0] == 0x01)
        {
            _setError(MachineError::OutSignalBlocked);
            if (auto* iface = _safeGetInterface()) iface->onError(CMD, responseListData);
            return false;
        }
        if (responseListData[0] == 0x02)
        {
            _setError(MachineError::ImmediateSignalBlocked);
            return true;
        }
        if (responseListData[0] == 0x03)
        {
            _setError(MachineError::StateUnchanged);
            if (auto* iface = _safeGetInterface()) iface->onError(CMD, responseListData);
            return false;
        }
        _setError(MachineError::UnknownError);
        if (auto* iface = _safeGetInterface()) iface->onError(CMD, responseListData);
        return false;
    }
    if (auto* iface = _safeGetInterface()) iface->onError(CMD, responseListData);
    return false;
}

std::vector<uint8_t> MachinePrivate::_buildSendFrame(const FrameData& frameData)
{
    std::vector<uint8_t> data;
    data.reserve(CMD_LEN);
    uint16_t len = htons(frameData.s_FrameDataList.size());
    uint64_t registerNo = m_RegisterNo.load();
    data.insert(data.end(), reinterpret_cast<const uint8_t*>(&registerNo), reinterpret_cast<const uint8_t*>(&registerNo)+sizeof(registerNo));
    data.insert(data.end(), reinterpret_cast<const uint8_t*>(&frameData.s_Command), reinterpret_cast<const uint8_t*>(&frameData.s_Command)+sizeof(frameData.s_Command));
    data.insert(data.end(), reinterpret_cast<const uint8_t*>(&len), reinterpret_cast<const uint8_t*>(&len)+sizeof(len));
    data.insert(data.end(), frameData.s_FrameDataList.begin(), frameData.s_FrameDataList.end());
    uint8_t checkSum = Unity::getChecksum(data);
    data.emplace_back(checkSum);
    return data;
}

MachineError MachinePrivate::_parseResponseFrame(const std::vector<uint8_t>& response, uint16_t command, FrameData& out)
{
    out.s_Command = command;

    if (response.size() < CMD_LEN)
    {
        return MachineError::ResponseTooShort;
    }
    if (response.back() != Unity::getChecksum({response.begin(), response.end() - 1}))
    {
        return MachineError::ChecksumMismatch;
    }

    uint64_t readRegisterNo = 0;
    memcpy(&readRegisterNo, response.data(), 8);
    uint64_t expectedNo = m_RegisterNo.load();
    if (expectedNo == 0)
    {
        m_RegisterNo.store(readRegisterNo);
    }
    else if (readRegisterNo != expectedNo)
    {
        return MachineError::RegisterNoMismatch;
    }

    uint16_t readCommand = 0;
    memcpy(&readCommand, response.data() + 8, 2);
    if (readCommand != command)
    {
        return MachineError::CommandMismatch;
    }

    uint16_t readDataLen = 0;
    memcpy(&readDataLen, response.data() + 10, 2);
    readDataLen = htons(readDataLen);

    if (readCommand & 0x0010)
    {
        out.s_FrameDataList.push_back(response[12]);
    }
    else
    {
        out.s_FrameDataList.assign(response.begin() + 12, response.begin() + 12 + readDataLen);
    }

    return MachineError::None;
}

bool MachinePrivate::_requestCommand(uint16_t command, uint8_t byteData, std::vector<uint8_t>& responseListData)
{
    std::lock_guard<std::mutex> serialLock(m_RequestSerialMutex);

    _setError(MachineError::None);

    if (!m_IsRunning.load())
    {
        _setError(MachineError::Stopped);
        return false;
    }

    {
        std::lock_guard<std::mutex> rlock(m_ResponseMutex);
        while (!m_ResponseQueue.empty())
        {
            m_ResponseQueue.pop();
        }
    }

    FrameData request;

    request.s_Command = htons(command);
    request.s_FrameDataList.push_back(byteData);

    {
        std::lock_guard<std::mutex> lock(m_CommandMutex);
        m_CommandQueue.push(request);
    }
    m_CommandCond.notify_one();

    FrameData response;
    {
        std::unique_lock<std::mutex> lock(m_ResponseMutex);
        if (!m_ResponseCond.wait_for(lock, std::chrono::milliseconds(WAIT_MILLION_SECONDS), [this] {
            return !m_ResponseQueue.empty() || !m_IsRunning.load();
        }))
        {
            _setError(MachineError::Timeout);
            return false;
        }
        if (!m_IsRunning.load() && m_ResponseQueue.empty())
        {
            _setError(MachineError::Stopped);
            return false;
        }
        response = m_ResponseQueue.front();
        m_ResponseQueue.pop();
    }

    if (m_LastError.load() != MachineError::None)
    {
        return false;
    }

    responseListData = std::move(response.s_FrameDataList);
    return true;
}

bool MachinePrivate::_requestCommand(uint16_t command, std::vector<uint8_t> listData,std::vector<uint8_t>& responseListData)
{
    std::lock_guard<std::mutex> serialLock(m_RequestSerialMutex);

    _setError(MachineError::None);

    if (!m_IsRunning.load())
    {
        _setError(MachineError::Stopped);
        return false;
    }

    {
        std::lock_guard<std::mutex> rlock(m_ResponseMutex);
        while (!m_ResponseQueue.empty())
        {
            m_ResponseQueue.pop();
        }
    }

    FrameData request;

    request.s_Command = htons(command);
    request.s_FrameDataList = listData;

    {
        std::lock_guard<std::mutex> lock(m_CommandMutex);
        m_CommandQueue.push(request);
    }
    m_CommandCond.notify_one();

    FrameData response;
    {
        std::unique_lock<std::mutex> lock(m_ResponseMutex);
        if (!m_ResponseCond.wait_for(lock, std::chrono::milliseconds(WAIT_MILLION_SECONDS), [this] {
            return !m_ResponseQueue.empty() || !m_IsRunning.load();
        }))
        {
            _setError(MachineError::Timeout);
            return false;
        }
        if (!m_IsRunning.load() && m_ResponseQueue.empty())
        {
            _setError(MachineError::Stopped);
            return false;
        }
        response = m_ResponseQueue.front();
        m_ResponseQueue.pop();
    }

    if (m_LastError.load() != MachineError::None)
    {
        return false;
    }

    responseListData = std::move(response.s_FrameDataList);
    return true;
}

void MachinePrivate::run()
{
    m_IsRunning.store(true);
    mpRunThread = std::make_unique<std::thread>([this]
    {
        while (m_IsRunning.load())
        {
            FrameData request;
            {
                std::unique_lock<std::mutex> lock(m_CommandMutex);
                m_CommandCond.wait(lock, [this] {
                    return !m_CommandQueue.empty() || !m_IsRunning.load();
                });
                if (!m_IsRunning.load() && m_CommandQueue.empty())
                {
                    break;
                }
                request = std::move(m_CommandQueue.front());
                m_CommandQueue.pop();
            }

            if (nullptr == mpPort)
            {
                _setError(MachineError::PortNotOpen);
                FrameData flag;
                std::lock_guard<std::mutex> rlock(m_ResponseMutex);
                m_ResponseQueue.push(flag);
                m_ResponseCond.notify_one();
                continue;
            }

            std::vector<uint8_t> sendData = _buildSendFrame(request);
            int ret = mpPort->write(sendData.data(), sendData.size());
            if (ret != static_cast<int>(sendData.size()))
            {
                _setError(MachineError::WriteFailed);
                FrameData flag;
                std::lock_guard<std::mutex> rlock(m_ResponseMutex);
                m_ResponseQueue.push(flag);
                m_ResponseCond.notify_one();
                continue;
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
                cycle--;
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }

            std::vector<uint8_t> rawResponse(pData, pData + retLen);
            FrameData parsed;
            MachineError parseErr = _parseResponseFrame(rawResponse, request.s_Command, parsed);
            if (parseErr != MachineError::None)
            {
                _setError(parseErr);
                FrameData flag;
                std::lock_guard<std::mutex> rlock(m_ResponseMutex);
                m_ResponseQueue.push(flag);
                m_ResponseCond.notify_one();
                continue;
            }

            {
                std::lock_guard<std::mutex> rlock(m_ResponseMutex);
                m_ResponseQueue.push(parsed);
            }
            m_ResponseCond.notify_one();
        }
    });
}

void MachinePrivate::stop()
{
    m_IsRunning.store(false);
    m_CommandCond.notify_all();
    m_ResponseCond.notify_all();
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

void MachinePrivate::_setError(MachineError err)
{
    m_LastError.store(err);
}

const char* MachinePrivate::getLastErrorInfo()
{
    static const char* None = "No error";
    static const char* PortNotOpen = "Port is not open";
    static const char* WriteFailed = "Failed to write to port";
    static const char* ResponseTooShort = "Response data is too short";
    static const char* ChecksumMismatch = "Checksum mismatch";
    static const char* RegisterNoMismatch = "Register number mismatch";
    static const char* CommandMismatch = "Command mismatch";
    static const char* Timeout = "Request timeout";
    static const char* Stopped = "Machine has stopped";
    static const char* ConnectFailed = "Failed to connect port";
    static const char* JsonParseFailed = "Failed to parse JSON data";
    static const char* OutSignalBlocked = "Output signal blocked";
    static const char* ImmediateSignalBlocked = "Immediate signal blocked";
    static const char* StateUnchanged = "State unchanged";
    static const char* UnknownError = "Unknown error";

    switch (m_LastError.load())
    {
    case MachineError::None:               return None;
    case MachineError::PortNotOpen:        return PortNotOpen;
    case MachineError::WriteFailed:         return WriteFailed;
    case MachineError::ResponseTooShort:   return ResponseTooShort;
    case MachineError::ChecksumMismatch:   return ChecksumMismatch;
    case MachineError::RegisterNoMismatch: return RegisterNoMismatch;
    case MachineError::CommandMismatch:    return CommandMismatch;
    case MachineError::Timeout:            return Timeout;
    case MachineError::Stopped:            return Stopped;
    case MachineError::ConnectFailed:      return ConnectFailed;
    case MachineError::JsonParseFailed:    return JsonParseFailed;
    case MachineError::OutSignalBlocked: return OutSignalBlocked;
    case MachineError::ImmediateSignalBlocked: return ImmediateSignalBlocked;
    case MachineError::StateUnchanged: return StateUnchanged;
    case MachineError::UnknownError: return UnknownError;
    default:                               return "Unknown error";
    }
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

MachineDataInterface* MachinePrivate::_safeGetInterface()
{
    std::lock_guard<std::mutex> lock(m_MachineDataMutex);
    return mpMachineDataInterface;
}

const MachineData& MachinePrivate::getMachineData() const
{
    return m_MachineData;
}