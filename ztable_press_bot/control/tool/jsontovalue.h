/*
 * jsontovalue.h
 *
 *  Created on: 2026年8月30日
 *      Author: 11518
 */

#ifndef DATA_JSONTOVALUE_H_
#define DATA_JSONTOVALUE_H_

#include <json/json.h>

enum class RetValueCode
{
    VALUE_NO_ERROR,
    VALUE_NO_KEY,
    VALUE_TYPE_ERROR,
    VALUE_TYPE_OVERFLOW,
    VALUE_PASS_RANGE,
    VALUE_ERROR_SIZE,
    VALUE_ERROR_PARAMETER,
};

class JsonToValue
{
public:
    static RetValueCode getUInt8Value(const std::string& key, const Json::Value& root, uint8_t& value,
                                      uint8_t minLimitValue, uint8_t maxLimitValue);
    static RetValueCode getUInt32Value(const std::string& key, const Json::Value& root, uint32_t& value,
                                      uint32_t minLimitValue, uint32_t maxLimitValue);
    static RetValueCode getFloatValue(const std::string& key, const Json::Value& root, float& value,
                                      float minLimitValue, float maxLimitValue);
    static RetValueCode getFloatVector(const std::string& key, const Json::Value& root, std::vector<float>& value,
                                       float minLimitValue, float maxLimitValue, int maxSize);
    static RetValueCode getFloatVector(const std::string& key, const Json::Value& root, std::vector<float>& value,
                                       int minLimitValue, const std::vector<float>& maxLimitValue, int maxSize);
    static RetValueCode getUInt8Vector(const std::string& key, const Json::Value& root, std::vector<uint8_t>& value,
                                       uint8_t minLimitValue, uint8_t maxLimitValue, int maxSize);
    static RetValueCode getUInt32Vector(const std::string& key, const Json::Value& root, std::vector<uint32_t>& value,
                                        uint32_t minLimitValue, uint32_t maxLimitValue, int maxSize);
    static RetValueCode getStringValue(const std::string& key, const Json::Value& root, std::string& value);
private:
    static RetValueCode _getUInt8Value(const std::string& key, const Json::Value& root, uint8_t& value);

    static RetValueCode _getUInt32Value(const std::string& key, const Json::Value& root, uint32_t& value);

    static RetValueCode _getFloatValue(const std::string& key, const Json::Value& root, float& value);
    static RetValueCode _getFloatVector(const std::string& key, const Json::Value& root, std::vector<float>& value,
                                        int maxSize);
    static RetValueCode _getUInt8Vector(const std::string& key, const Json::Value& root, std::vector<uint8_t>& value,
                                        int maxSize);
    static RetValueCode _getUInt32Vector(const std::string& key, const Json::Value& root, std::vector<uint32_t>& value,
                                         int maxSize);
};

#endif /* DATA_JSONTOVALUE_H_ */
