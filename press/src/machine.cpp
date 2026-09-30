//
// Created by 11518 on 2026/9/13.
//

#include "press.hpp"
#include "machineprivate.h"
#include "version.h"


Machine::Machine() : mpPrivate(std::make_unique<MachinePrivate>())
{
}

Machine::~Machine() = default;

bool Machine::connect(const char* portName, PortType portType)
{
    return mpPrivate->connect(portName, portType);
}

void Machine::disconnect()
{
    mpPrivate->disconnect();
}

bool Machine::isConnected() const
{
    return mpPrivate->isConnected();
}

uint64_t Machine::getMachineRegisterNo() const
{
    return mpPrivate->getMachineRegisterNo();
}

const char* Machine::getLastErrorInfo() const
{
    return mpPrivate->getLastErrorInfo();
}

void Machine::run()
{
    mpPrivate->run();
}

void Machine::stop()
{
    mpPrivate->stop();
}

bool Machine::isRunning() const
{
    return mpPrivate->isRunning();
}

bool Machine::getReadOnlyData(ReadOnlyData& data, bool isCompressed)
{
    return mpPrivate->getReadOnlyData(data, isCompressed);
}

bool Machine::getPressData(PressData& data, bool isCompressed)
{
    return mpPrivate->getPressData(data, isCompressed);
}

bool Machine::setPressData(const PressData& data, bool isCompressed)
{
    return mpPrivate->setPressData(data, isCompressed);
}

bool Machine::getRealTimeData(RealTimeData& data, bool isCompressed)
{
    return mpPrivate->getRealTimeData(data, isCompressed);
}

bool Machine::setPressing(bool isPressing)
{
    return mpPrivate->setPressing(isPressing);
}

bool Machine::setDemolding(bool isDemolding)
{
    return mpPrivate->setDemolding(isDemolding);
}

void Machine::registerDataInterface(MachineDataInterface* dataInterface)
{
    mpPrivate->registerDataInterface(dataInterface);
}

void Machine::unregisterDataInterface()
{
    mpPrivate->unregisterDataInterface();
}

std::vector<std::string> Machine::getPortList()
{
    return MachinePrivate::getPortList();
}

std::string Machine::version()
{
    return PRESS_VERSION;
}