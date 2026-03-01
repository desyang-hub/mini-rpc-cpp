#pragma once

#include <string>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <mutex>

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
    mutable int m_sockfd;
    mutable std::mutex m_mutex;
public:
    RpcChannel(std::string ip, int port) : _ip(ip), _port(port), m_sockfd{-1} {};
    virtual ~RpcChannel() {};

private:
    bool EnsureConnected() const {
        if (m_sockfd != -1) {
            return true;
        }

        // 重新建立连接
        m_sockfd = Connect();
        return true;
    }

    // 建立连接
    int Connect() const {
        // 1. 创建socket
        m_sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (m_sockfd == -1) {
            perror("socket error");
            exit(1);
        }

        // 2. 设置服务器地址
        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
        server_addr.sin_port = htons(_port); // 端口类型转换

        // 3. 向服务器发起连接请求
        if (connect(m_sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
            perror("connect error");
            close(m_sockfd);
            exit(1);
        }   
        
        LOG_INFO("success connect to Server");

        return m_sockfd;
    }

public:
    void CallFunc(const std::string &svc, const std::string &mth, const json &req, json &res) const {

        std::lock_guard<std::mutex> lock(m_mutex);

        // 1. 连接到rcp_server
        if (!EnsureConnected()) {
            throw std::runtime_error("Faild to Connect");
        }

        // 2. 构造请求
        const RpcRequest request{svc, mth, req};

        // 3. 根据通信协议(protocol)进行数据包封装
        const std::string bytes = Protocol::RequestEncode(request);

        LOG_INFO("req: %s", req.dump().c_str());

        int d = 0;

        // 4. 发送数据报
        if ((d = send(m_sockfd, bytes.data(), bytes.size(), 0)) < 0) {
            // 异常
            if (errno != EINTR) {
                perror("send error");
                close(m_sockfd);
                throw std::runtime_error("Send data error");
            }
        }

        // 5. 接收回复
        try
        {
            const RpcResponse response = Protocol::ResponseDecode(m_sockfd);
            res = response.data;
        }
        catch(const std::exception& e)
        {
            // 尝试重连
            m_sockfd = -1;
        }
    }


    void CallFunc(const std::string &svc, const std::string &mth, const std::string& req, std::string &resp) const {

        std::lock_guard<std::mutex> lock(m_mutex);

        // 1. 连接到rcp_server
        if (!EnsureConnected()) {
            throw std::runtime_error("Faild to Connect");
        }

        // 2. 构造请求
        // const RpcRequest request{svc, mth, req};

        // 3. 根据通信协议(protocol)进行数据包封装
        const std::string bytes = Protocol::Encode({svc, mth, req});

        int d = 0;

        // 4. 发送数据报
        if ((d = send(m_sockfd, bytes.data(), bytes.size(), 0)) < 0) {
            // 异常
            if (errno != EINTR) {
                perror("send error");
                close(m_sockfd);
                throw std::runtime_error("Send data error");
            }
        }

        // 5. 接收回复
        try
        {
            // 这里回复只需要读取一次就好了
            const std::string bytes = Protocol::Decode(m_sockfd);
            resp = bytes;
        }
        catch(const std::exception& e)
        {
            // 尝试重连
            m_sockfd = -1;
        }
    }
};


} // namespace minirpc
