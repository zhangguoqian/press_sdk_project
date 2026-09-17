//
// Created by 11518 on 2026/9/13.
//

#ifndef ZTABLE_PRESS_BOT_PROJECT_PORTBASE_H
#define ZTABLE_PRESS_BOT_PROJECT_PORTBASE_H
#include <cstdint>


class PortBase
{
public:
    PortBase();
    virtual ~PortBase();
    virtual void setPortName(const char* portName) = 0;
    virtual bool open() = 0;
    virtual void close() = 0;
    virtual int write(const uint8_t* data, int len) = 0;
    virtual int read(uint8_t* data, int len) = 0;
    virtual bool isOpen() = 0;
};


#endif //ZTABLE_PRESS_BOT_PROJECT_PORTBASE_H
