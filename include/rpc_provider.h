#pragma once

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <functional>
#include <map>
#include <unistd.h>

#include "log.h"
#include "nlohmann/json.hpp"
#include "protocol.h"
#include "constants.h"
#include "thread_pool/thread_pool.h"

#ifndef BUFFER_SIZE
#define BUFFER_SIZE 1024
#endif

using json = nlohmann::json;

// rpc 服务提供

namespace minirpc
{

// 定义Rpc服务处理类型
using RpcHandler = std::function<void(const json&, json&)>;
    
class RpcProvider
{
private:
    /* data */
    std::map<std::string, RpcHandler> m_handlers;
    int _port;
    ThreadPool m_pool;

public:
    RpcProvider(int port = 8080);
    ~RpcProvider();

private:
    void OnMessage(const int sockfd) const;

public:
    // 注册方法
    void RegisterMethod(const std::string &svc, const std::string mth, const RpcHandler &handler);

    // 服务启动，对外开放连接处理请求,进入阻塞状态
    void Run();
};

RpcProvider::RpcProvider(int port) : _port(port)
{
}

RpcProvider::~RpcProvider()
{
}

// 将服务函数进行注册
void RpcProvider::RegisterMethod(const std::string &svc, const std::string mth, const RpcHandler &handler) {
    std::string key = svc + "." + mth;
    m_handlers[key] = handler;

    LOG_INFO("Register method %s", key.c_str());
}

// 处理用户请求
void RpcProvider::OnMessage(const int sockfd) const {
    // 读取数据， 假如一次读取完成
    char buffer[BUFFER_SIZE];
    while (true) {
        memset(buffer, 0, BUFFER_SIZE); // 重置缓冲区        

        RpcRequest request;
        try
        {
            request = Protocol::RequestDecode(sockfd);
        }
        catch(const std::exception& e)
        {
            LOG_ERROR("Client bad request");

            json bed_request("{'code':0, 'msg': 'Bad Request'}");

            const std::string bytes = Protocol::ResponseEncode(RpcResponse{bed_request});
            int n;
            if ((n = send(sockfd, bytes.data(), bytes.size(), 0)) <= 0) {
                if (n == 0) {
                    break;
                }
                close(sockfd);
                throw std::runtime_error("send data error");
            }
        }


        // 如果字段为空，证明正常退出
        if (request.serviceName.empty()) {
            break;
        }

        // 进行处理
        json response;

        std::string key = request.serviceName + '.' + request.methodName;

        // 调用方法进行处理
        if (m_handlers.find(key) != m_handlers.end()) {
            m_handlers.at(key)(request.data, response);

            // 将结果返回到客户端
            const std::string bytes = Protocol::ResponseEncode(RpcResponse{response});

            if (send(sockfd, bytes.data(), bytes.size(), 0) <= 0) {
                close(sockfd);
                throw std::runtime_error("send data error");
            }
        }
        else {
            LOG_ERROR("Not found func %s", key.c_str());
            continue;
        }
         
        
    }

    close(sockfd);
}


void RpcProvider::Run() {
    // 1. 创建socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("failed create socket.");
        exit(1);
    }

    // 2. 设置地址复用，避免端口占用
    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 3. 绑定
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // 监听所有网卡请求
    address.sin_port = htons(_port); // 设置监听端口 TODO: 必须转换数据类型

    if (bind(sockfd, (struct sockaddr*)&address, sizeof(address)) == -1) {
        perror("bind error");
        close(sockfd);
        exit(1);
    }

    // 4. listen 监听
    if (listen(sockfd, 3) == -1) { // listen 第二个参数的含义是当连接数小于设置值时会进入等待队列，若超过该值，则直接拒绝服务请求
        perror("listen error");
        close(sockfd);
        exit(1);
    }

    LOG_INFO("Listen net port: %d", _port);

    int client_fd;
    while (true) {
        // 5. accept 接受请求并服务
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        client_fd = accept(sockfd, (struct sockaddr*)&client_addr, &client_len);
        
        if (client_fd == -1) {
            perror("accept error");
            close(sockfd);
            close(client_fd);
            exit(1);
        }

        LOG_INFO("Client connected: %s", inet_ntoa(client_addr.sin_addr));

        // 将任务提交到线程池执行
        m_pool.submit([this, client_fd](){
            OnMessage(client_fd);
        });
    }

    

    // 7. 关闭socket
    close(sockfd);
    close(client_fd);
}


} // namespace minirpc
