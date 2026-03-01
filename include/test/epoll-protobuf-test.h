#pragma once

#include "test/util.h"
#include "protocol/user_service_impl.h"
#include "rpc_provider_epoll_protobuf.h"
#include "rpc_channel.h"
#include "service/user_service_stub.h"

namespace epoll_protobuf_test
{
void run_server();
void run_client();

void test() {
    test_(run_server, run_client);
}

void run_server() {
    UserServiceImpl service;
    RpcProviderEpollProtobuf provider;

    fixbug::UserServiceImpl fixbug_service;

    // 注册 "UserService.Login" 方法
    provider.RegisterMethod("UserService", "Login", 
        [&service](const json& req, json& resp) {
            service.Login(req, resp);
        });

    provider.RegisterMethod("UserService", &fixbug_service);
    
    provider.EpollRun();
}    


void run_client() {
    sleep(2);
    std::cout << "\n=== Client Start (Real RPC Protobuf) ===" << std::endl;

    try {
        // 1. 创建通道
        RpcChannel channel("127.0.0.1", 8080);

        // 2. 创建桩 (Stub)
        UserServiceStub stub(&channel);

        // 3. 准备登录数据
        fixbug::LoginRequest request;
        request.set_name("zhangsan");
        request.set_pwd("123456");

        fixbug::LoginResponse response;

        for (int i = 0; i < 10; ++i) {
            json resp_json;
            response.set_code(1);

            // 4. 【关键】调用远程方法 (像本地一样！)
            // stub.Login(req_json, resp_json);
            stub.LoginProtobuf(request, response);
    
            // 5. 处理结果
            if (response.code() == 0) {
                std::cout << "✅ SUCCESS: " << response.msg() << std::endl;
            } else {
                std::cout << "❌ FAILED: " << response.msg() << std::endl;
            }
        }    

    } catch (std::exception& e) {
        std::cerr << "RPC Error: " << e.what() << std::endl;
    }
}
} // namespace epoll_protobuf_test
