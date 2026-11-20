#pragma once

#include "core/press_service.h"

#include <string>

/**
 * 设备 JSON 报文解析（独立于 QObject，便于单元测试）。
 * 输入为 SDK 回调或 get*Json 返回的原始 JSON 字符串。
 */
namespace PressJson {

PressReadOnlyData parseReadOnly(const std::string &json);
PressProgramData parsePressProgram(const std::string &json);
PressTempProgramData parseTempProgram(const std::string &json);
PressMixProgramData parseMixProgram(const std::string &json);
PressRtData parseRealTime(const std::string &json);

} // namespace PressJson
