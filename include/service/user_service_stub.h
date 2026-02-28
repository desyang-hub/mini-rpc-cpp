#pragma once

#include "service/user_service.h"
#include "rpc_channel.h"

using namespace minirpc;

class UserServiceStub : public IUserService
{
private:
    RpcChannel m_channel;
public:
UserServiceStub(const RpcChannel& channel);
    void Login(const json& req, json& resp) const;
};

UserServiceStub::UserServiceStub(const RpcChannel& channel) : m_channel(channel) {

}

void UserServiceStub::Login(const json& req, json& resp) const {
    m_channel.CallFunc("UserService", "Login", req, resp);
}