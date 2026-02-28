#include "rpc_channel.h"
#include "rpc_provider.h"
#include "service/user_service_impl.h"
#include "service/user_service_stub.h"

#include <iostream>
#include <thread>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using json = nlohmann::json;
using namespace minirpc;

// ================= 服务端逻辑 =================
void run_server() {
    UserServiceImpl service;
    RpcProvider provider;

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

    provider.Run(); // 启动服务器 (阻塞)
}

// ================= 客户端逻辑 =================
void run_client() {
    sleep(2);
    std::cout << "\n=== Client Start (Real RPC) ===" << std::endl;

    try {
        // 1. 创建通道
        RpcChannel channel("127.0.0.1", 8080);

        // 2. 创建桩 (Stub)
        UserServiceStub stub(channel);

        // 3. 准备数据 (纯 C++ 对象)
        json req_json;
        req_json["name"] = "zhangsan";
        req_json["pwd"] = "123456";
        
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

    } catch (std::exception& e) {
        std::cerr << "RPC Error: " << e.what() << std::endl;
    }
}

// ================= 主函数 =================
int main() {
    // 开启两个线程：一个做服务器，一个做客户端
    std::thread t1(run_server);
    std::thread t2(run_client);

    t1.join();
    t2.join();

    return 0;
}