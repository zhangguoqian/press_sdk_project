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
#include <unordered_map>


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
    bool getRealTimeData(RealTimeData& data,uint8_t isCompressed = 0);

    int runCommand();

    void run();
    void stop();
    bool isRunning() const;

    static std::vector<std::string> getPortList();
    const char* getLastErrorInfo();

    void registerDataInterface(MachineDataInterface* dataInterface);
    void unregisterDataInterface();

private:
    std::vector<uint8_t> _combinedSendData(uint64_t &registerNo,uint16_t &command);
    int _isValidResponseData(std::vector<uint8_t>& response,uint64_t registerNo,uint16_t command);
    bool _commandByteData(uint16_t command,uint8_t byteData,std::vector<uint8_t> &responseListData);


    char m_ErrorInfo[128] = {0};

    std::unique_ptr<PortBase> mpPort = nullptr;

    std::atomic<uint64_t> m_RegisterNo{0};       //!< 机器注册号

    std::atomic<bool> m_IsHasCommand{false};       //!< 是否有命令
    std::atomic<uint16_t> m_Command{0};          //!< 命令
    std::atomic<uint8_t> m_CommandByteData{0};    //!< 命令数据获取
    std::vector<uint8_t> m_CommandListData = {};     //!< 命令数据设置
    std::mutex m_SetCommandMutex{};                 //!< 命令数据设置互斥锁

    std::atomic<uint8_t> m_ResponseByteData{0};   //!< 响应字节数据
    std::vector<uint8_t> m_ResponseListData = {};    //!< 响应列表数据
    std::mutex m_GetResponseDataMutex{};            //!< 响应数据队列互斥锁
    std::condition_variable m_GetResponseDataCond{};

    std::unique_ptr<std::thread> mpRunThread = nullptr; //!< 运行线程
    std::atomic<bool> m_IsRunning = false; //!< 是否运行中

    std::atomic<int> m_RunCode{-1};
    std::atomic<bool> m_ResponseReady{false};

    MachineData m_MachineData{};


    MachineDataInterface* mpMachineDataInterface = nullptr; //!< 数据接口
    std::mutex m_MachineDataMutex{}; //!< 数据接口互斥锁
};

#endif