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

bool Press::getDeviceVersionInfo(VersionInfo& versionInfo) const
{
    return mpPrivate->getCachedVersionInfo(versionInfo);
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

bool Press::getReadOnlyDataJson(std::string& json, bool isCompressed)
{
    return mpPrivate->getReadOnlyDataJson(json, isCompressed);
}

bool Press::getSerialNumber(std::string& serialNumber)
{
    return mpPrivate->getSerialNumber(serialNumber);
}

void Press::setRequestTimeout(unsigned int ms)
{
    mpPrivate->setRequestTimeoutMs(static_cast<std::uint32_t>(ms));
}



bool Press::getPressDataJson(std::string& json, bool isCompressed)
{
    return mpPrivate->getPressDataJson(json, isCompressed);
}

bool Press::setPressDataJson(const std::string& json, bool isCompressed)
{
    return mpPrivate->setPressDataJson(json, isCompressed);
}

bool Press::getRealTimeDataJson(std::string& json, bool isCompressed)
{
    return mpPrivate->getRealTimeDataJson(json, isCompressed);
}

bool Press::setPressing(bool isPressing)
{
    return mpPrivate->setPressing(isPressing);
}

bool Press::setDemolding(bool isDemolding)
{
    return mpPrivate->setDemolding(isDemolding);
}

bool Press::startAir()
{
    return mpPrivate->startAir();
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

// ==================== 压力参数命令（单值/索引） ====================

bool Press::getPressStepCount(uint8_t& step)
{
    return mpPrivate->getPressStepCount(step);
}

bool Press::setPressStepCount(uint8_t step)
{
    return mpPrivate->setPressStepCount(step);
}

bool Press::getCalibrationValue(float& value)
{
    return mpPrivate->getCalibrationValue(value);
}

bool Press::setCalibrationValue(float value)
{
    return mpPrivate->setCalibrationValue(value);
}

bool Press::getMoldType(uint8_t& type)
{
    return mpPrivate->getMoldType(type);
}

bool Press::setMoldType(uint8_t type)
{
    return mpPrivate->setMoldType(type);
}

bool Press::getCircle(float& diameter)
{
    return mpPrivate->getCircle(diameter);
}

bool Press::setCircle(float diameter)
{
    return mpPrivate->setCircle(diameter);
}

bool Press::getRectangle(float& a, float& b)
{
    return mpPrivate->getRectangle(a, b);
}

bool Press::setRectangle(float a, float b)
{
    return mpPrivate->setRectangle(a, b);
}

bool Press::getAirTime(uint8_t& time)
{
    return mpPrivate->getAirTime(time);
}

bool Press::setAirTime(uint8_t time)
{
    return mpPrivate->setAirTime(time);
}

bool Press::getPressValueIndex(uint8_t index, float& value)
{
    return mpPrivate->getPressValueIndex(index, value);
}

bool Press::setPressValueIndex(uint8_t index, float value)
{
    return mpPrivate->setPressValueIndex(index, value);
}

bool Press::getAfterValueIndex(uint8_t index, float& value)
{
    return mpPrivate->getAfterValueIndex(index, value);
}

bool Press::setAfterValueIndex(uint8_t index, float value)
{
    return mpPrivate->setAfterValueIndex(index, value);
}

bool Press::getKeepPressTime(uint8_t index, int32_t& time)
{
    return mpPrivate->getKeepPressTime(index, time);
}

bool Press::setKeepPressTime(uint8_t index, int32_t time)
{
    return mpPrivate->setKeepPressTime(index, time);
}

// ==================== 温度参数命令（单值/索引） ====================

bool Press::getTempStepCount(uint8_t& step)
{
    return mpPrivate->getTempStepCount(step);
}

bool Press::setTempStepCount(uint8_t step)
{
    return mpPrivate->setTempStepCount(step);
}

bool Press::getTempSpeedMold(uint8_t& mold)
{
    return mpPrivate->getTempSpeedMold(mold);
}

bool Press::setTempSpeedMold(uint8_t mold)
{
    return mpPrivate->setTempSpeedMold(mold);
}

bool Press::getStartTempIndex(uint8_t index, float& value)
{
    return mpPrivate->getStartTempIndex(index, value);
}

bool Press::setTempIndex(uint8_t index, float value)
{
    return mpPrivate->setTempIndex(index, value);
}

bool Press::getTempSpeedIndex(uint8_t index, float& value)
{
    return mpPrivate->getTempSpeedIndex(index, value);
}

bool Press::setTempSpeedIndex(uint8_t index, float value)
{
    return mpPrivate->setTempSpeedIndex(index, value);
}

bool Press::getKeepTempTime(uint8_t index, int32_t& time)
{
    return mpPrivate->getKeepTempTime(index, time);
}

bool Press::setKeepTempTime(uint8_t index, int32_t time)
{
    return mpPrivate->setKeepTempTime(index, time);
}

bool Press::getMaxWaterValue(float& value)
{
    return mpPrivate->getMaxWaterValue(value);
}

bool Press::setMaxWaterValue(float value)
{
    return mpPrivate->setMaxWaterValue(value);
}

bool Press::getMinWaterValue(float& value)
{
    return mpPrivate->getMinWaterValue(value);
}

bool Press::setMinWaterValue(float value)
{
    return mpPrivate->setMinWaterValue(value);
}

bool Press::getMixStepCount(uint8_t& step)
{
    return mpPrivate->getMixStepCount(step);
}

bool Press::setMixStepCount(uint8_t step)
{
    return mpPrivate->setMixStepCount(step);
}

bool Press::getMixMoldType(uint8_t& type)
{
    return mpPrivate->getMixMoldType(type);
}

bool Press::setMixMoldType(uint8_t type)
{
    return mpPrivate->setMixMoldType(type);
}

bool Press::getMixRectangle(float& a, float& b)
{
    return mpPrivate->getMixRectangle(a, b);
}

bool Press::setMixRectangle(float a, float b)
{
    return mpPrivate->setMixRectangle(a, b);
}

bool Press::getMixCircle(float& radius)
{
    return mpPrivate->getMixCircle(radius);
}

bool Press::setMixCircle(float radius)
{
    return mpPrivate->setMixCircle(radius);
}

bool Press::getMixIsPressIndex(uint8_t index, bool& isPress)
{
    return mpPrivate->getMixIsPressIndex(index, isPress);
}

bool Press::setMixIsPressIndex(uint8_t index, bool isPress)
{
    return mpPrivate->setMixIsPressIndex(index, isPress);
}

bool Press::getMixPressIndex(uint8_t index, float& value)
{
    return mpPrivate->getMixPressIndex(index, value);
}

bool Press::setMixPressIndex(uint8_t index, float value)
{
    return mpPrivate->setMixPressIndex(index, value);
}

bool Press::getMixAfterPressIndex(uint8_t index, float& value)
{
    return mpPrivate->getMixAfterPressIndex(index, value);
}

bool Press::setMixAfterPressIndex(uint8_t index, float value)
{
    return mpPrivate->setMixAfterPressIndex(index, value);
}

bool Press::getMixIsHeatUpIndex(uint8_t index, bool& isHeatUp)
{
    return mpPrivate->getMixIsHeatUpIndex(index, isHeatUp);
}

bool Press::setMixIsHeatUpIndex(uint8_t index, bool isHeatUp)
{
    return mpPrivate->setMixIsHeatUpIndex(index, isHeatUp);
}

bool Press::getMixHeatUpIndex(uint8_t index, float& value)
{
    return mpPrivate->getMixHeatUpIndex(index, value);
}

bool Press::setMixHeatUpIndex(uint8_t index, float value)
{
    return mpPrivate->setMixHeatUpIndex(index, value);
}

bool Press::getMixIsHeatDownIndex(uint8_t index, bool& isHeatDown)
{
    return mpPrivate->getMixIsHeatDownIndex(index, isHeatDown);
}

bool Press::setMixIsHeatDownIndex(uint8_t index, bool isHeatDown)
{
    return mpPrivate->setMixIsHeatDownIndex(index, isHeatDown);
}

bool Press::getMixHeatDownIndex(uint8_t index, float& value)
{
    return mpPrivate->getMixHeatDownIndex(index, value);
}

bool Press::setMixHeatDownIndex(uint8_t index, float value)
{
    return mpPrivate->setMixHeatDownIndex(index, value);
}

bool Press::getMixKeepTime(uint8_t index, int32_t& time)
{
    return mpPrivate->getMixKeepTime(index, time);
}

bool Press::setMixKeepTime(uint8_t index, int32_t time)
{
    return mpPrivate->setMixKeepTime(index, time);
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

bool Press::getTempDataJson(std::string& json, bool isCompressed)
{
    return mpPrivate->getTempDataJson(json, isCompressed);
}

bool Press::setTempDataJson(const std::string& json, bool isCompressed)
{
    return mpPrivate->setTempDataJson(json, isCompressed);
}

bool Press::getMixDataJson(std::string& json, bool isCompressed)
{
    return mpPrivate->getMixDataJson(json, isCompressed);
}

bool Press::setMixDataJson(const std::string& json, bool isCompressed)
{
    return mpPrivate->setMixDataJson(json, isCompressed);
}

void Press::registerDataInterface(PressDataInterface* dataInterface, int intervalSeconds)
{
    mpPrivate->registerDataInterface(dataInterface, intervalSeconds);
}

void Press::registerDataInterfaceOwned(std::shared_ptr<PressDataInterface> dataInterface, int intervalSeconds)
{
    mpPrivate->registerDataInterfaceOwned(std::move(dataInterface), intervalSeconds);
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