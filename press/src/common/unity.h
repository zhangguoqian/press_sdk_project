//
// Created by 11518 on 2026/9/13.
//

#ifndef PRESS_SDK_PROJECT_UNITY_H
#define PRESS_SDK_PROJECT_UNITY_H

#include <cstdint>
#include <vector>
#include <string>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

class Unity
{
public:
    //! @brief 计算校验和
    //! @param data [in] the data to calculate the checksum
    //! @return uint8_t the checksum
    static uint8_t getChecksum(const std::vector<uint8_t>& data);

    //! @brief 计算校验和（迭代器范围版本，避免创建临时 vector）
    template <typename InputIt>
    static uint8_t getChecksum(InputIt first, InputIt last)
    {
        uint8_t checksum = 0;
        for (auto it = first; it != last; ++it)
        {
            checksum += static_cast<uint8_t>(*it);
        }
        return checksum;
    }

    static std::string getCurrentTime();
};


#endif //PRESS_SDK_PROJECT_UNITY_H