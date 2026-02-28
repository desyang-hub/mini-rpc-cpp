#pragma once

#include "service/user_service.h"
#include "rpc_channel.h"
#include "log.h"

using namespace minirpc;

class UserServiceImpl : public IUserService
{
public:
    UserServiceImpl() {}
    void Login(const json& req, json& resp) const;
};

void UserServiceImpl::Login(const json& request, json& response) const {
    // 1. 从 JSON 中获取参数 (如果 key 不存在，返回默认值)
    std::string name = request.value("name", "");
    std::string pwd = request.value("pwd", "");

    LOG_INFO("[Business] Processing Login: %s / %s", name.c_str(), pwd.c_str());

    // 2. 模拟数据库验证
    if (name == "zhangsan" && pwd == "123456") {
        response["code"] = 0;
        response["msg"] = "Login successful!";
        response["token"] = "abc-123-xyz-token";
    } else {
        response["code"] = 1;
        response["msg"] = "Invalid username or password.";
    }
}