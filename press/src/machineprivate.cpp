//
// Created by 11518 on 2026/9/13.
//
#include "machineprivate.h"
#include "machine.h"
#include "port/serialport.h"
#include "port/tcpsocket.h"
#include "common/unity.h"
#include <cstring>
#include <thread>

#ifdef _WIN32
#   include <winsock2.h>
#else
#   include <arpa/inet.h>
#endif

namespace {
constexpr uint64_t WAIT_MILLI_SECONDS = 300;
constexpr int CMD_LEN = 14;

const char* errorToString(MachineError err)
{
    switch (err)
    {
    case MachineError::None:                return "No error";
    case MachineError::PortNotOpen:         return "Port is not open";
    case MachineError::WriteFailed:         return "Failed to write to port";
    case MachineError::ResponseTooShort:    return "Response data is too short";
    case MachineError::ChecksumMismatch:    return "Checksum mismatch";
    case MachineError::RegisterNoMismatch: return "Register number mismatch";
    case MachineError::CommandMismatch:     return "Command mismatch";
    case MachineError::Timeout:             return "Request timeout";
    case MachineError::Stopped:             return "Machine has stopped";
    case MachineError::ConnectFailed:       return "Failed to connect port";
    case MachineError::JsonParseFailed:     return "Failed to parse JSON data";
    case MachineError::OutSignalBlocked:    return "Output signal blocked";
    case MachineError::ImmediateSignalBlocked: return "Immediate signal blocked";
    case MachineError::StateUnchanged:      return "State unchanged";
    case MachineError::UnknownError:        return "Unknown error";
    default:                                return "Unknown error";
    }
}
} // namespace

MachinePrivate::MachinePrivate() = default;

MachinePrivate::~MachinePrivate()
{
    stop();
}

bool MachinePrivate::connect(const char* portName, PortType portType)
{
    if (mpPort == nullptr)
    {
        if (portType == SerialPortType)
        {
            mpPort = std::make_unique<SerialPort>();
        }
        else if (portType == TcpSocketPortType)
        {
            mpPort = std::make_unique<TcpSocket>();
        }
    }
    mpPort->setPortName(portName);
    bool ret = mpPort->open();
    if (!ret)
    {
        _setError(MachineError::ConnectFailed);
    }
    return ret;
}

void MachinePrivate::disconnect()
{
    if (mpPort != nullptr)
    {
        mpPort->close();
    }
}

bool MachinePrivate::isConnected() const
{
    if (mpPort == nullptr)
    {
        return false;
    }
    return mpPort->isOpen();
}

uint64_t MachinePrivate::getMachineRegisterNo() const
{
    return m_RegisterNo;
}

bool MachinePrivate::getReadOnlyData(ReadOnlyData& data, bool isCompressed)
{
    std::vector<uint8_t> responseListData;
    if (!_requestCommand(GET_ROD_JSON, {static_cast<uint8_t>(isCompressed)}, responseListData))
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

bool MachinePrivate::getPressData(PressData& data, bool isCompressed)
{
    std::vector<uint8_t> responseListData;
    if (!_requestCommand(GET_PD_JSON, {static_cast<uint8_t>(isCompressed)}, responseListData))
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

bool MachinePrivate::setPressData(const PressData& data, bool isCompressed)
{
    std::vector<uint8_t> responseListData;
    std::vector<uint8_t> commandListData;
    commandListData.emplace_back(static_cast<uint8_t>(isCompressed));
    std::vector<uint8_t> tempDataList = data.toFrameData();
    commandListData.insert(commandListData.end(), tempDataList.begin(), tempDataList.end());
    if (!_requestCommand(SET_PD_JSON, commandListData, responseListData))
    {
        if (auto* iface = _safeGetInterface()) iface->onError(SET_PD_JSON, responseListData);
        return false;
    }
    return _handleSetResponse(SET_PD_JSON, responseListData);
}

bool MachinePrivate::getRealTimeData(RealTimeData& data, bool isCompressed)
{
    std::vector<uint8_t> responseListData;
    if (!_requestCommand(GET_RT_JSON, {static_cast<uint8_t>(isCompressed)}, responseListData))
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

bool MachinePrivate::setPressing(bool isPressing)
{
    std::vector<uint8_t> responseListData;
    if (!_requestCommand(SET_PRESS, {static_cast<uint8_t>(isPressing)}, responseListData))
    {
        if (auto* iface = _safeGetInterface()) iface->onError(SET_PRESS, responseListData);
        return false;
    }
    return _handleSetResponse(SET_PRESS, responseListData);
}

bool MachinePrivate::setDemolding(bool isDemolding)
{
    std::vector<uint8_t> responseListData;
    if (!_requestCommand(SET_DEMOLD, {static_cast<uint8_t>(isDemolding)}, responseListData))
    {
        if (auto* iface = _safeGetInterface()) iface->onError(SET_DEMOLD, responseListData);
        return false;
    }
    return _handleSetResponse(SET_DEMOLD, responseListData);
}

bool MachinePrivate::_handleSetResponse(uint16_t cmd, const std::vector<uint8_t>& responseListData)
{
    if (responseListData.size() == 1)
    {
        switch (responseListData[0])
        {
        case 0x00: return true;
        case 0x01:
            _setError(MachineError::OutSignalBlocked);
            if (auto* iface = _safeGetInterface()) iface->onError(cmd, responseListData);
            return false;
        case 0x02:
            _setError(MachineError::ImmediateSignalBlocked);
            return true;
        case 0x03:
            _setError(MachineError::StateUnchanged);
            if (auto* iface = _safeGetInterface()) iface->onError(cmd, responseListData);
            return false;
        default:
            _setError(MachineError::UnknownError);
            if (auto* iface = _safeGetInterface()) iface->onError(cmd, responseListData);
            return false;
        }
    }
    if (auto* iface = _safeGetInterface()) iface->onError(cmd, responseListData);
    return false;
}

std::vector<uint8_t> MachinePrivate::_buildSendFrame(const FrameData& frameData)
{
    std::vector<uint8_t> data;
    data.reserve(CMD_LEN + frameData.m_FrameDataList.size());

    uint64_t registerNo = m_RegisterNo.load();
    data.insert(data.end(), reinterpret_cast<const uint8_t*>(&registerNo),
                reinterpret_cast<const uint8_t*>(&registerNo) + sizeof(registerNo));

    uint16_t cmd = frameData.m_Command;
    data.insert(data.end(), reinterpret_cast<const uint8_t*>(&cmd),
                reinterpret_cast<const uint8_t*>(&cmd) + sizeof(cmd));

    uint16_t len = htons(static_cast<uint16_t>(frameData.m_FrameDataList.size()));
    data.insert(data.end(), reinterpret_cast<const uint8_t*>(&len),
                reinterpret_cast<const uint8_t*>(&len) + sizeof(len));

    data.insert(data.end(), frameData.m_FrameDataList.begin(), frameData.m_FrameDataList.end());

    uint8_t checkSum = Unity::getChecksum(data);
    data.emplace_back(checkSum);
    return data;
}

MachineError MachinePrivate::_parseResponseFrame(const std::vector<uint8_t>& response, uint16_t command, FrameData& out)
{
    out.m_Command = command;

    if (response.size() < CMD_LEN)
    {
        return MachineError::ResponseTooShort;
    }
    if (response.back() != Unity::getChecksum({response.begin(), response.end() - 1}))
    {
        return MachineError::ChecksumMismatch;
    }

    uint64_t readRegisterNo = 0;
    std::memcpy(&readRegisterNo, response.data(), sizeof(readRegisterNo));
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
    std::memcpy(&readCommand, response.data() + 8, sizeof(readCommand));
    if (readCommand != command)
    {
        return MachineError::CommandMismatch;
    }

    uint16_t readDataLen = 0;
    std::memcpy(&readDataLen, response.data() + 10, sizeof(readDataLen));
    readDataLen = htons(readDataLen);

    if (readCommand & 0x0010)
    {
        out.m_FrameDataList.push_back(response[12]);
    }
    else
    {
        out.m_FrameDataList.assign(response.begin() + 12, response.begin() + 12 + readDataLen);
    }

    return MachineError::None;
}

bool MachinePrivate::_requestCommand(uint16_t command, const std::vector<uint8_t>& listData, std::vector<uint8_t>& responseListData)
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
    request.m_Command = htons(command);
    request.m_FrameDataList = listData;

    {
        std::lock_guard<std::mutex> lock(m_CommandMutex);
        m_CommandQueue.push(std::move(request));
    }
    m_CommandCond.notify_one();

    FrameData response;
    {
        std::unique_lock<std::mutex> lock(m_ResponseMutex);
        if (!m_ResponseCond.wait_for(lock, std::chrono::milliseconds(WAIT_MILLI_SECONDS), [this] {
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
        response = std::move(m_ResponseQueue.front());
        m_ResponseQueue.pop();
    }

    if (m_LastError.load() != MachineError::None)
    {
        return false;
    }

    responseListData = std::move(response.m_FrameDataList);
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

            if (mpPort == nullptr)
            {
                _setError(MachineError::PortNotOpen);
                FrameData flag;
                std::lock_guard<std::mutex> rlock(m_ResponseMutex);
                m_ResponseQueue.push(std::move(flag));
                m_ResponseCond.notify_one();
                continue;
            }

            std::vector<uint8_t> sendData = _buildSendFrame(request);
            auto writeRet = mpPort->write(sendData.data(), sendData.size());
            if (writeRet != sendData.size())
            {
                _setError(MachineError::WriteFailed);
                FrameData flag;
                std::lock_guard<std::mutex> rlock(m_ResponseMutex);
                m_ResponseQueue.push(std::move(flag));
                m_ResponseCond.notify_one();
                continue;
            }

            uint8_t pData[1024];
            int cycle = 10;
            int retLen = 0;
            
            while (cycle > 0)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                auto readRet = mpPort->read(pData + retLen, sizeof(pData) - retLen);
                if (readRet > 0)
                {
                    retLen += static_cast<int>(readRet);
                }
                if (retLen >= CMD_LEN)
                {
                    uint16_t len = (pData[10] << 8) | pData[11];
                    if (retLen >= CMD_LEN + len)
                    {
                        std::cout << "retLen: " << retLen << ", len: " << len << std::endl;
                        break;
                    }
                }
                cycle--;
            }

            std::vector<uint8_t> rawResponse(pData, pData + retLen);
            FrameData parsed;
            MachineError parseErr = _parseResponseFrame(rawResponse, request.m_Command, parsed);
            if (parseErr != MachineError::None)
            {
                _setError(parseErr);
                FrameData flag;
                std::lock_guard<std::mutex> rlock(m_ResponseMutex);
                m_ResponseQueue.push(std::move(flag));
                m_ResponseCond.notify_one();
                continue;
            }

            {
                std::lock_guard<std::mutex> rlock(m_ResponseMutex);
                m_ResponseQueue.push(std::move(parsed));
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
    auto list = SerialPort::listAvailablePorts();
    std::vector<std::string> portList;
    portList.reserve(list.size());
    for (const auto& port : list)
        portList.emplace_back(port.name);
    return portList;
}

void MachinePrivate::_setError(MachineError err)
{
    m_LastError.store(err);
}

const char* MachinePrivate::getLastErrorInfo()
{
    return errorToString(m_LastError.load());
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