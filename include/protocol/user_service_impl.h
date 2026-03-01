// include/user_service_impl.h
#pragma once

#include "protocol/user_service.h"

namespace fixbug {

// 具体实现类
class UserServiceImpl : public UserService {
    public:
        void Login(
            google::protobuf::RpcController* controller,
            const LoginRequest* request,
            LoginResponse* response,
            google::protobuf::Closure* done) const override;
                   
        void GetUserInfo(
            google::protobuf::RpcController *controller,
            const GetUserInfoRequest *request,
            GetUserInfoResponse * response,
            google::protobuf::Closure *closure) const override;
};

void UserServiceImpl::Login(
google::protobuf::RpcController* controller,
const LoginRequest* request,
LoginResponse* response,
google::protobuf::Closure* done) const {
    // 模拟查询数据库
    if (request->name() == "zhangsan" && request->pwd() == "123456") {
        response->set_code(0);
        response->set_msg("login successful.");
    }
    else {
        response->set_code(1);
        response->set_msg("name or password error");
    }

    if(done) done->Run();
}

void UserServiceImpl::GetUserInfo(
google::protobuf::RpcController *controller,
const GetUserInfoRequest *request,
GetUserInfoResponse * response,
google::protobuf::Closure *closure) const {
    if (request->id() == 1) {
        response->set_code(10086);
        response->set_name("xiaoming");
        response->set_is_vip(1);
    }   
    else {
        response->set_code(0);
        response->set_name("");
        response->set_is_vip(0);
    }
}


} // namespace fixbug