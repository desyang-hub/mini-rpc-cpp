#pragma once

#include <string>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "nlohmann/json.hpp"
#include "protocol.h"
#include "log.h"

using json = nlohmann::json;

#ifndef BUFFER_SIZE
#define BUFFER_SIZE 1024
#endif


namespace minirpc
{

class RpcChannel
{
private:
    std::string  _ip;
    int _port;
public:
    RpcChannel(std::string ip, int port) : _ip(ip), _port(port) {};
    virtual ~RpcChannel() {};

private:
    // 建立连接
    int Connect() const {
        // 1. 创建socket
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd == -1) {
            perror("socket error");
            exit(1);
        }

        // 2. 设置服务器地址
        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
        server_addr.sin_port = htons(_port); // 端口类型转换

        // 3. 向服务器发起连接请求
        if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
            perror("connect error");
            close(sockfd);
            exit(1);
        }   
        
        LOG_INFO("success connect to Server");

        return sockfd;
    }

public:
    void CallFunc(const std::string &svc, const std::string &mth, const json &req, json &res) const {
        // 1. 连接到rcp_server
        int server_fd = Connect();

        // 2. 构造请求
        const RpcRequest request{svc, mth, req};

        // 3. 根据通信协议(protocol)进行数据包封装
        const std::string bytes = Protocol::RequestEncode(request);

        int d = 0;

        // 4. 发送数据报
        if ((d = send(server_fd, bytes.data(), bytes.size(), 0)) <= 0) {
            close(server_fd);
            throw std::runtime_error("Send data error");
        }

        // 5. 接收回复
        const RpcResponse response = Protocol::ResponseDecode(server_fd);
        res = response.data;

        // 6. 关闭连接
        close(server_fd);
    }
};


} // namespace minirpc
