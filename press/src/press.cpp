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

bool Press::connect(const char* portName, PortType portType)
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

const char* Press::getLastErrorInfo() const
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

bool Press::getReadOnlyData(ReadOnlyData& data, bool isCompressed)
{
    return mpPrivate->getReadOnlyData(data, isCompressed);
}

bool Press::getPressData(PressData& data, bool isCompressed)
{
    return mpPrivate->getPressData(data, isCompressed);
}

bool Press::setPressData(const PressData& data, bool isCompressed)
{
    return mpPrivate->setPressData(data, isCompressed);
}

bool Press::getRealTimeData(RealTimeData& data, bool isCompressed)
{
    return mpPrivate->getRealTimeData(data, isCompressed);
}

bool Press::setPressing(bool isPressing)
{
    return mpPrivate->setPressing(isPressing);
}

bool Press::setDemolding(bool isDemolding)
{
    return mpPrivate->setDemolding(isDemolding);
}

void Press::registerDataInterface(PressDataInterface* dataInterface)
{
    mpPrivate->registerDataInterface(dataInterface);
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