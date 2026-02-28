#include "interface/serializer.h"

namespace minirpc {

class JsonSerializer : public Serializer
{
public:
    std::string Serialization(const json& msg_json) const;
    json Deserialization(const std::string msg) const;
};

std::string JsonSerializer::Serialization(const json& msg_json) const {
    return msg_json.dump();
}

json JsonSerializer::Deserialization(const std::string msg) const {
    return json::parse(msg);
}

} // namespace end minirpc
