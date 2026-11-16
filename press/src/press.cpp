//
// Created by 11518 on 2026/9/13.
//

#include "press.hpp"
#include "pressprivate.h"
#include "version.h"


Press::Press() : mpPrivate(std::make_unique<PressPrivate>())
{
}

Press::~Press() = default;

bool Press::connect(const char* portName, PressPortType portType)
{
    return mpPrivate->connect(portName, portType);
}

void Press::disconnect()
{
    mpPrivate->disconnect();
}

bool Press::isConnected() const
{
    return mpPrivate->isConnected();
}

uint64_t Press::getMachineRegisterNo() const
{
    return mpPrivate->getMachineRegisterNo();
}

const char* Press::getLastErrorInfo()
{
    return mpPrivate->getLastErrorInfo();
}

void Press::run()
{
    mpPrivate->run();
}

void Press::stop()
{
    mpPrivate->stop();
}

bool Press::isRunning() const
{
    return mpPrivate->isRunning();
}

bool Press::getReadOnlyDataJson(ReadOnlyData& data, bool isCompressed)
{
    return mpPrivate->getReadOnlyDataJson(data, isCompressed);
}

bool Press::getSerialNumber(std::string& serialNumber)
{
    return mpPrivate->getSerialNumber(serialNumber);
}

void Press::setRequestTimeout(unsigned int ms)
{
    mpPrivate->setRequestTimeoutMs(static_cast<std::uint32_t>(ms));
}



bool Press::getPressDataJson(PressData& data, bool isCompressed)
{
    return mpPrivate->getPressDataJson(data, isCompressed);
}

bool Press::setPressDataJson(const PressData& data, bool isCompressed)
{
    return mpPrivate->setPressDataJson(data, isCompressed);
}

bool Press::getRealTimeDataJson(RealTimeData& data, bool isCompressed)
{
    return mpPrivate->getRealTimeDataJson(data, isCompressed);
}

bool Press::setPressing(bool isPressing)
{
    return mpPrivate->setPressing(isPressing);
}

bool Press::setDemolding(bool isDemolding)
{
    return mpPrivate->setDemolding(isDemolding);
}

// ==================== 设备信息与参数块 ====================

bool Press::getName(std::string& name, uint8_t language)
{
    return mpPrivate->getName(name, language);
}

bool Press::getMachineType(std::string& type)
{
    return mpPrivate->getMachineType(type);
}

bool Press::getPressParameter(PressParameter& param)
{
    return mpPrivate->getPressParameter(param);
}

bool Press::getTempParameter(TempParameter& param)
{
    return mpPrivate->getTempParameter(param);
}

bool Press::getOtherInfo(OtherInfo& info)
{
    return mpPrivate->getOtherInfo(info);
}

bool Press::getPressStepParameter(uint8_t index, PressStepParameter& param)
{
    return mpPrivate->getPressStepParameter(index, param);
}

bool Press::setPressStepParameter(uint8_t index, const PressStepParameter& param)
{
    return mpPrivate->setPressStepParameter(index, param);
}

bool Press::getTempStepParameter(uint8_t index, TempStepParameter& param)
{
    return mpPrivate->getTempStepParameter(index, param);
}

bool Press::setTempStepParameter(uint8_t index, const TempStepParameter& param)
{
    return mpPrivate->setTempStepParameter(index, param);
}

bool Press::getMixStepParameter(uint8_t index, MixStepParameter& param)
{
    return mpPrivate->getMixStepParameter(index, param);
}

bool Press::setMixStepParameter(uint8_t index, const MixStepParameter& param)
{
    return mpPrivate->setMixStepParameter(index, param);
}

// ==================== 系统命令 ====================

bool Press::getDateTime(DateTimeInfo& dateTime)
{
    return mpPrivate->getDateTime(dateTime);
}

bool Press::setDateTime(const DateTimeInfo& dateTime)
{
    return mpPrivate->setDateTime(dateTime);
}

bool Press::getBrightness(uint8_t& brightness)
{
    return mpPrivate->getBrightness(brightness);
}

bool Press::setBrightness(uint8_t brightness)
{
    return mpPrivate->setBrightness(brightness);
}

bool Press::getButtonSound(bool& enabled)
{
    return mpPrivate->getButtonSound(enabled);
}

bool Press::setButtonSound(bool enabled)
{
    return mpPrivate->setButtonSound(enabled);
}

bool Press::getLanguage(uint8_t& language)
{
    return mpPrivate->getLanguage(language);
}

bool Press::setLanguage(uint8_t language)
{
    return mpPrivate->setLanguage(language);
}

// ==================== 状态与控制命令 ====================

bool Press::getMachineState(MachineState& state)
{
    return mpPrivate->getMachineState(state);
}

bool Press::setStartHeat(bool isHeating)
{
    return mpPrivate->setStartHeat(isHeating);
}

bool Press::setStart(bool isStart)
{
    return mpPrivate->setStart(isStart);
}

bool Press::selectModelState(uint8_t model)
{
    return mpPrivate->selectModelState(model);
}

// ==================== JSON 批量命令（字符串形式） ====================

bool Press::getTempDataJson(std::string& json)
{
    return mpPrivate->getTempDataJson(json);
}

bool Press::setTempDataJson(const std::string& json)
{
    return mpPrivate->setTempDataJson(json);
}

bool Press::getMixDataJson(std::string& json)
{
    return mpPrivate->getMixDataJson(json);
}

bool Press::setMixDataJson(const std::string& json)
{
    return mpPrivate->setMixDataJson(json);
}

void Press::registerDataInterface(PressDataInterface* dataInterface, int intervalSeconds)
{
    mpPrivate->registerDataInterface(dataInterface, intervalSeconds);
}

void Press::unregisterDataInterface()
{
    mpPrivate->unregisterDataInterface();
}

std::vector<std::string> Press::getPortList()
{
    return PressPrivate::getPortList();
}

std::string Press::version()
{
    return PRESS_VERSION;
}