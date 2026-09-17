//
// Created by 11518 on 2026/9/13.
//

#ifndef PRESS_BOT_PROJECT_MACHINE_H
#define PRESS_BOT_PROJECT_MACHINE_H

#include <vector>
#include <memory>
#include "machinetype.h"

class MachinePrivate;


class Machine
{
public:
    //! @brief 构造函数
    Machine();
    Machine(const Machine& other) = delete;
    Machine& operator=(const Machine& other) = delete;
    Machine(Machine&& other) = delete;
    Machine& operator=(Machine&& other) = delete;
    //! @brief 析构函数
    ~Machine();
    //! @brief 连接机器
    bool connect(const char* portName) const;
    //! @brief 断开机器连接
    void disconnect() const;
    //! @brief 判断机器是否连接
    //! @return bool 是否连接
    bool isConnected() const;
    //! @brief 获取机器注册号
    //! @return uint64_t the机器注册号
    uint64_t getMachineRegisterNo() const;
    //! @brief 设置命令
    //! @param command [in] the command the 命令
    //! @param data [in] the data the 数据
    //! @param len [in] the length of data the 数据的长度
    void setCommand(uint16_t command,const std::vector<uint8_t> &data) const;
    //! @brief 获取命令
    //! @return std::vector<uint8_t> the 响应
    std::vector<uint8_t> getResponse() const;
    //! @brief 获取最后错误信息
    //! @return char* the 最后一个错误信息
    const char* getLastErrorInfo() const;
    //! @brief 设置运行命令
    //! @param command [in] the command the 命令
    //! @param data [in] the data the 数据
    //! @param len [in] the length of data the 数据的长度
    //! @return int the 错误码
    int runCommand(const std::pair<uint16_t, std::vector<uint8_t>>& commandData) const;
    //! @brief 设置并运行命令
    //! @param command [in] the command the 命令
    //! @param data [in] the data the 数据
    //! @param len [in] the length of data the 数据的长度
    //! @return int the 错误码
    int runCommand(uint16_t command, const std::vector<uint8_t> &data) const;
    //! @brief 运行命令
    //! @return int the 错误码
    //! @note 运行命令后，需要调用getResponse()获取响应
    //! @return int the 错误码
    //! @note 错误码为0表示成功
    int runCommand() const;
    //! @brief 添加命令到队列
    //! @param commandData [in] the commandData the 命令数据
    //! @return int the 错误码
    //! @note 错误码为0表示成功
    void addCommand(const std::pair<uint16_t, std::vector<uint8_t>>& commandData) const;

    //! @brief 运行机器
    void run() const;
    //! @brief 停止机器
    void stop() const;
    //! @brief 判断机器是否运行
    //! @return bool 是否运行
    bool isRunning() const;

    //! @brief 注册数据接口
    //! @param dataInterface [in] the dataInterface the 数据接口
    void registerDataInterface(MachineDataInterface* dataInterface) const;
    //! @brief 注销数据接口
    void unregisterDataInterface() const;

    //! @brief 获取端口列表
    //! @return std::vector<std::string> the 端口列表
    static std::vector<std::string> getPortList();
private:
    std::unique_ptr<MachinePrivate> mpPrivate;
};
#endif