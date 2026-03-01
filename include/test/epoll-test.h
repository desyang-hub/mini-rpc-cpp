#pragma once
#include "test/util.h"
#include "service/user_service_impl.h"
#include "service/user_service_stub.h"
#include "rpc_provider_epoll.h"

namespace epoll_test
{

using json = nlohmann::json;

void run_server();
void run_client();

void test() {
    test_(run_server, run_client);
}

// ================= 服务端逻辑 =================
void run_server() {
    UserServiceImpl service;
    RpcProviderEpoll provider;

    // 注册 "UserService.Login" 方法
    provider.RegisterMethod("UserService", "Login", 
        [&service](const json& req, json& resp) {
            service.Login(req, resp);
        });
    
    // 注册 "UserService.GetUserInfo" 方法
    // provider.RegisterMethod("UserService", "GetUserInfo",
    //     [&service](const json& req, json& resp) {
    //         service.GetUserInfo(req, resp);
    //     });

    // provider.Run(); // 启动服务器 (阻塞)
    provider.EpollRun();
}


// ================= 客户端逻辑 =================
void run_client() {
    sleep(2);
    std::cout << "\n=== Client Start (Real RPC) ===" << std::endl;

    try {
        // 1. 创建通道
        RpcChannel channel("127.0.0.1", 8080);

        // 2. 创建桩 (Stub)
        UserServiceStub stub(&channel);

        // 3. 准备数据 (纯 C++ 对象)
        json req_json;
        req_json["name"] = "zhangsan";
        req_json["pwd"] = "123456";


        for (int i = 0; i < 10; ++i) {
            json resp_json;

            // 4. 【关键】调用远程方法 (像本地一样！)
            stub.Login(req_json, resp_json);
    
            // 5. 处理结果
            if (resp_json["code"] == 0) {
                std::cout << "✅ SUCCESS: " << resp_json["msg"] << std::endl;
                std::cout << "   Token: " << resp_json["token"] << std::endl;
            } else {
                std::cout << "❌ FAILED: " << resp_json["msg"] << std::endl;
            }
        }    

    } catch (std::exception& e) {
        std::cerr << "RPC Error: " << e.what() << std::endl;
    }
}
    
} // namespace name
