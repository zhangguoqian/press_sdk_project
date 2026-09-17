//
// Created by 11518 on 2026/9/13.
//

#ifndef ZTABLE_PRESS_BOT_PROJECT_UNITY_H
#define ZTABLE_PRESS_BOT_PROJECT_UNITY_H

#include <cstdint>
#include <vector>

class Unity
{
public:
    //! @brief 计算校验和
    //! @param data [in] the data to calculate the checksum
    //! @return uint8_t the checksum
    static uint8_t getChecksum(const std::vector<uint8_t>& data);
};


#endif //ZTABLE_PRESS_BOT_PROJECT_UNITY_H
