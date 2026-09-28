//
// Created by 11518 on 2026/9/13.
//

#include "machine.h"
#include "machineprivate.h"


Machine::Machine() : mpPrivate(std::make_unique<MachinePrivate>())
{
}

Machine::~Machine()
{

};

bool Machine::connect(const char* portName) const
{
    return mpPrivate->connect(portName);
}

void Machine::disconnect() const
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


void Machine::run() const
{
    mpPrivate->run();
}

void Machine::stop() const
{
    mpPrivate->stop();
}

bool Machine::isRunning() const
{
    return mpPrivate->isRunning();
}

bool Machine::getReadOnlyData(ReadOnlyData& data, uint8_t isCompressed)
{
    return mpPrivate->getReadOnlyData(data, isCompressed);
}

bool Machine::getPressData(PressData& data, uint8_t isCompressed)
{
    return mpPrivate->getPressData(data, isCompressed);
}

bool Machine::setPressData(const PressData& data, uint8_t isCompressed)
{
    return mpPrivate->setPressData(data, isCompressed);
}

bool Machine::getRealTimeData(RealTimeData& data, uint8_t isCompressed)
{
    return mpPrivate->getRealTimeData(data, isCompressed);
}

bool Machine::setPressing(uint8_t isPressing)
{
    return mpPrivate->setPressing(isPressing);
}

bool Machine::setDemolding(uint8_t isDemolding)
{
    return mpPrivate->setDemolding(isDemolding);
}

void Machine::registerDataInterface(MachineDataInterface* dataInterface) const
{
    mpPrivate->registerDataInterface(dataInterface);
}

void Machine::unregisterDataInterface() const
{
    mpPrivate->unregisterDataInterface();
}

std::vector<std::string> Machine::getPortList()
{
    return MachinePrivate::getPortList();
}
