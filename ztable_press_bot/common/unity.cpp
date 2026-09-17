//
// Created by 11518 on 2026/9/13.
//

#include "unity.h"

uint8_t Unity::getChecksum(const std::vector<uint8_t>& data)
{
    uint8_t checksum = 0;
    for (auto& item : data)
    {
        checksum += item;
    }
    return checksum;
}
