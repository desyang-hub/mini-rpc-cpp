#pragma once

#include "nlohmann/json.hpp"

using json = nlohmann::json;

class IUserService
{
public:
    virtual ~IUserService() {}
    virtual void Login(const json& req, json& resp) const = 0;
};
