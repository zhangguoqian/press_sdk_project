//
// Created by 11518 on 2026/9/13.
//
#ifndef PRESS_BOT_PROJECT_MACHINEPRIVATE_H
#define PRESS_BOT_PROJECT_MACHINEPRIVATE_H

#include "port/portbase.h"
#include "machinedata.h"
#include <memory>
#include <vector>
#include <cstdint>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>


enum class MachineError
{
    None = 0,
    PortNotOpen,
    WriteFailed,
    ResponseTooShort,
    ChecksumMismatch,
    RegisterNoMismatch,
    CommandMismatch,
    Timeout,
    Stopped,
    ConnectFailed,
    JsonParseFailed,

    OutSignalBlocked,
    ImmediateSignalBlocked,
    StateUnchanged,

    UnknownError,
};

struct FrameData
{
    uint16_t s_Command = 0;
    std::vector<uint8_t> s_FrameDataList = {};
};

class MachinePrivate
{
public:
    MachinePrivate();
    ~MachinePrivate();

    bool connect(const char* portName);
    void disconnect() const;
    bool isConnected() const;
    uint64_t getMachineRegisterNo() const;

    bool getReadOnlyData(ReadOnlyData& data,uint8_t isCompressed = 0);
    bool getPressData(PressData& data,uint8_t isCompressed = 0);
    bool setPressData(const PressData& data,uint8_t isCompressed = 0);
    bool getRealTimeData(RealTimeData& data,uint8_t isCompressed = 0);
    bool setPressing(uint8_t isPressing);
    bool setDemolding(uint8_t isDemolding);

    void run();
    void stop();
    bool isRunning() const;

    static std::vector<std::string> getPortList();
    const char* getLastErrorInfo();

    void registerDataInterface(MachineDataInterface* dataInterface);
    void unregisterDataInterface();

    const MachineData& getMachineData() const;

private:
    std::vector<uint8_t> _buildSendFrame(const FrameData& frameData);
    MachineError _parseResponseFrame(const std::vector<uint8_t>& response, uint16_t command, FrameData& out);
    bool _requestCommand(uint16_t command, uint8_t byteData, std::vector<uint8_t>& responseListData);
    bool _requestCommand(uint16_t command, std::vector<uint8_t> listData, std::vector<uint8_t>& responseListData);
    void _setError(MachineError err);
    MachineDataInterface* _safeGetInterface();

    std::mutex m_RequestSerialMutex{};

    std::mutex m_CommandMutex{};
    std::queue<FrameData> m_CommandQueue = {};
    std::condition_variable m_CommandCond{};

    std::mutex m_ResponseMutex{};
    std::queue<FrameData> m_ResponseQueue = {};
    std::condition_variable m_ResponseCond{};

    std::unique_ptr<std::thread> mpRunThread = nullptr;
    std::atomic<bool> m_IsRunning = false;

    std::atomic<uint64_t> m_RegisterNo{0};
    std::unique_ptr<PortBase> mpPort = nullptr;

    std::atomic<MachineError> m_LastError{MachineError::None};

    MachineData m_MachineData{};
    MachineDataInterface* mpMachineDataInterface = nullptr;
    std::mutex m_MachineDataMutex{};
};

#endif