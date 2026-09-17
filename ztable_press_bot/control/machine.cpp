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

void Machine::setCommand(uint16_t command, const std::vector<uint8_t>& data) const
{
    mpPrivate->setCommand(command, data);
}

std::vector<uint8_t> Machine::getResponse() const
{
    return mpPrivate->getResponse();
}

const char* Machine::getLastErrorInfo() const
{
    return mpPrivate->getLastErrorInfo();
}

int Machine::runCommand(const std::pair<uint16_t, std::vector<uint8_t>>& commandData) const
{
    return mpPrivate->runCommand(commandData);
}

int Machine::runCommand(uint16_t command, const std::vector<uint8_t>& data) const
{
    return mpPrivate->runCommand(command, data);
}

int Machine::runCommand() const
{
    return mpPrivate->runCommand();
}

void Machine::addCommand(const std::pair<uint16_t, std::vector<uint8_t>>& commandData) const
{
    mpPrivate->addCommand(commandData);
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
