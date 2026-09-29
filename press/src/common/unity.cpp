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

std::string Unity::getCurrentTime()
{
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&t);

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S")
        << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}