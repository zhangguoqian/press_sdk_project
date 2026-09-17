/*
 * jsontovalue.cpp
 *
 *  Created on: 2026年8月30日
 *      Author: 11518
 */

#include "jsontovalue.h"


RetValueCode JsonToValue::_getUInt8Value(const std::string& key, const Json::Value& root, uint8_t& value)
{
    if (!root.isMember(key))
    {
        return RetValueCode::VALUE_NO_KEY;
    }
    Json::Value obj = root[key];
    if (!obj.isUInt())
    {
        return RetValueCode::VALUE_TYPE_ERROR;
    }

    uint32_t temp = root[key].asUInt();
    if (temp > UCHAR_MAX)
    {
        return RetValueCode::VALUE_TYPE_OVERFLOW;
    }
    value = temp;
    return RetValueCode::VALUE_NO_ERROR;
}

RetValueCode JsonToValue::_getUInt32Value(const std::string& key, const Json::Value& root, uint32_t& value)
{
    if (!root.isMember(key))
    {
        return RetValueCode::VALUE_NO_KEY;
    }
    Json::Value obj = root[key];
    if (!obj.isUInt())
    {
        return RetValueCode::VALUE_TYPE_ERROR;
    }

    value = root[key].asUInt();
    return RetValueCode::VALUE_NO_ERROR;
}

RetValueCode JsonToValue::getUInt8Value(const std::string& key, const Json::Value& root, uint8_t& value,
                                        uint8_t minLimitValue, uint8_t maxLimitValue)
{
    RetValueCode code = _getUInt8Value(key, root, value);
    if (code == RetValueCode::VALUE_NO_ERROR)
    {
        if (value < minLimitValue || value > maxLimitValue)
        {
            return RetValueCode::VALUE_PASS_RANGE;
        }
    }
    return code;
}

RetValueCode JsonToValue::getUInt32Value(const std::string& key, const Json::Value& root, uint32_t& value,
    uint32_t minLimitValue, uint32_t maxLimitValue)
{
    RetValueCode code = _getUInt32Value(key, root, value);
    if (code == RetValueCode::VALUE_NO_ERROR)
    {
        if (value < minLimitValue || value > maxLimitValue)
        {
            return RetValueCode::VALUE_PASS_RANGE;
        }
    }
    return code;
}


RetValueCode JsonToValue::_getFloatValue(const std::string& key, const Json::Value& root, float& value)
{
    if (!root.isMember(key))
    {
        return RetValueCode::VALUE_NO_KEY;
    }
    Json::Value obj = root[key];
    if (!obj.isDouble())
    {
        return RetValueCode::VALUE_TYPE_ERROR;
    }

    value = root[key].asFloat();
    return RetValueCode::VALUE_NO_ERROR;
}

RetValueCode JsonToValue::getFloatValue(const std::string& key, const Json::Value& root, float& value,
                                        float minLimitValue, float maxLimitValue)
{
    RetValueCode code = _getFloatValue(key, root, value);
    if (code == RetValueCode::VALUE_NO_ERROR)
    {
        if (value < minLimitValue || value > maxLimitValue)
        {
            return RetValueCode::VALUE_PASS_RANGE;
        }
    }
    return code;
}

RetValueCode JsonToValue::_getFloatVector(const std::string& key, const Json::Value& root, std::vector<float>& value,
                                          int maxSize)
{
    if (!root.isMember(key))
    {
        return RetValueCode::VALUE_NO_KEY;
    }
    Json::Value obj = root[key];
    if (!obj.isArray())
    {
        return RetValueCode::VALUE_TYPE_ERROR;
    }
    if (obj.size() != maxSize)
    {
        return RetValueCode::VALUE_ERROR_SIZE;
    }
    value.clear();
    for (const auto& var : obj)
    {
        value.push_back(var.asFloat());
    }
    return RetValueCode::VALUE_NO_ERROR;
}

RetValueCode JsonToValue::getFloatVector(const std::string& key, const Json::Value& root, std::vector<float>& value,
                                         float minLimitValue, float maxLimitValue, int maxSize)
{
    RetValueCode code = _getFloatVector(key, root, value, maxSize);
    if (code == RetValueCode::VALUE_NO_ERROR)
    {
        for (size_t i = 0; i < value.size(); i++)
        {
            if (value[i] < minLimitValue || value[i] > maxLimitValue)
            {
                return RetValueCode::VALUE_PASS_RANGE;
            }
        }
    }
    return code;
}

RetValueCode JsonToValue::getFloatVector(const std::string& key, const Json::Value& root, std::vector<float>& value,
                                         int minLimitValue, const std::vector<float>& maxLimitValue, int maxSize)
{
    RetValueCode code = _getFloatVector(key, root, value, maxSize);
    if (maxLimitValue.size() != maxSize)
    {
        return RetValueCode::VALUE_ERROR_PARAMETER;
    }
    if (code == RetValueCode::VALUE_NO_ERROR)
    {
        for (size_t i = 0; i < value.size(); i++)
        {
            if (value[i] < minLimitValue || value[i] > maxLimitValue[i])
            {
                return RetValueCode::VALUE_PASS_RANGE;
            }
        }
    }
    return code;
}

RetValueCode JsonToValue::_getUInt32Vector(const std::string& key, const Json::Value& root,
                                           std::vector<uint32_t>& value, int maxSize)
{
    if (!root.isMember(key))
    {
        return RetValueCode::VALUE_NO_KEY;
    }
    Json::Value obj = root[key];
    if (!obj.isArray())
    {
        return RetValueCode::VALUE_TYPE_ERROR;
    }
    if (obj.size() != maxSize)
    {
        return RetValueCode::VALUE_ERROR_SIZE;
    }
    value.clear();
    for (const auto& var : obj)
    {
        value.push_back(var.asUInt());
    }
    return RetValueCode::VALUE_NO_ERROR;
}


RetValueCode JsonToValue::getUInt32Vector(const std::string& key, const Json::Value& root, std::vector<uint32_t>& value,
                                          uint32_t minLimitValue, uint32_t maxLimitValue, int maxSize)
{
    RetValueCode code = _getUInt32Vector(key, root, value, maxSize);
    if (code == RetValueCode::VALUE_NO_ERROR)
    {
        for (size_t i = 0; i < value.size(); i++)
        {
            if (value[i] < minLimitValue || value[i] > maxLimitValue)
            {
                return RetValueCode::VALUE_PASS_RANGE;
            }
        }
    }
    return code;
}

RetValueCode JsonToValue::getStringValue(const std::string& key, const Json::Value& root, std::string& value)
{
    if (!root.isMember(key))
    {
        return RetValueCode::VALUE_NO_KEY;
    }
    Json::Value obj = root[key];
    if (!obj.isString())
    {
        return RetValueCode::VALUE_TYPE_ERROR;
    }
    value = root[key].asString();
    return RetValueCode::VALUE_NO_ERROR;
}

RetValueCode JsonToValue::_getUInt8Vector(const std::string& key, const Json::Value& root, std::vector<uint8_t>& value,
                                          int maxSize)
{
    if (!root.isMember(key))
    {
        return RetValueCode::VALUE_NO_KEY;
    }
    Json::Value obj = root[key];
    if (!obj.isArray())
    {
        return RetValueCode::VALUE_TYPE_ERROR;
    }
    if (obj.size() != maxSize)
    {
        return RetValueCode::VALUE_ERROR_SIZE;
    }
    value.clear();
    for (const auto& var : obj)
    {
        value.push_back(var.asUInt());
    }
    return RetValueCode::VALUE_NO_ERROR;
}

RetValueCode JsonToValue::getUInt8Vector(const std::string& key, const Json::Value& root, std::vector<uint8_t>& value,
                                         uint8_t minLimitValue, uint8_t maxLimitValue, int maxSize)
{
    RetValueCode code = _getUInt8Vector(key, root, value, maxSize);
    if (code == RetValueCode::VALUE_NO_ERROR)
    {
        for (size_t i = 0; i < value.size(); i++)
        {
            if (value[i] < minLimitValue || value[i] > maxLimitValue)
            {
                return RetValueCode::VALUE_PASS_RANGE;
            }
        }
    }
    return code;
}
