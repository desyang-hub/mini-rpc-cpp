#pragma once

#include "nlohmann/json.hpp"

using json = nlohmann::json;

namespace minirpc {

// 定义序列化器接口，这样只需要实现不同的序列化器
class Serializer
{
public:
    // 序列化方法
    virtual std::string Serialization(const json& msg_json) const = 0;
    // 反序列化
    virtual json Deserialization(const std::string msg) const = 0;
};


} // namespace end minirpc