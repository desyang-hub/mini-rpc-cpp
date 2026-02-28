#pragma once

#include <string>
#include <sys/socket.h>
#include <unistd.h>

#include "log.h"
#include "nlohmann/json.hpp"
#include "interface/serializer.h"

using json = nlohmann::json;

namespace minirpc {

// 通信协议要做的就是编码和解码

typedef struct RpcRequest_
{
    std::string serviceName;
    std::string methodName;
    json data;
} RpcRequest;

typedef struct RpcResponse_
{
    json data;

    // RpcResponse_(int code, const std::string &msg) {
    //     data["code"] = code;
    //     data["msg"] = msg;
    // }

    // RpcResponse_(const json& resp) : data(resp) {
    // }
} RpcResponse;



// --- 协议解析 ---
// 格式：[服务名长度(4B)][服务名][方法名长度(4B)][方法名][JSON 长度 (4B)][JSON 数据]

// --- 发送响应 ---
// 格式：[JSON 长度 (4B)][JSON 数据]

// 使用一种类型来存放数据长度
using rpc_len = uint32_t;

class Protocol
{
private:
    static Serializer *m_serializer;

    static void append_str(std::string &src, const std::string &append) {
        // 32 位刚好是4字节，用于作为数据长度传递
        rpc_len len = append.size();
        // 加入数据长度 (4Byte)
        src.append((char*)&len, sizeof(rpc_len));
        // 加入数据 (data)
        src.append(append);
    }

    static std::string receive(int sockfd) {
        // 读取长度
        rpc_len len;

        int bytes_read;
        if ((bytes_read = recv(sockfd, (char*)&len, sizeof(len), 0)) <= 0) {
            // 断开连接
            if (bytes_read == 0) {
                LOG_INFO("Client disconnected");
                return "";
            }

            close(sockfd);
            throw std::runtime_error("Recv length error");
        }

        // 读取msg
        std::string msg(len, '\0');
        if (recv(sockfd, (char*)msg.data(), len, 0) <= 0) {
            close(sockfd);
            throw std::runtime_error("Recv data faild");
        }

        return msg;
    }

public:
    static const std::string RequestEncode(const RpcRequest& request) {
        std::string req_buf;

        // 1. 解析服务名长度和数据
        append_str(req_buf, request.serviceName);
        append_str(req_buf, request.methodName);
        append_str(req_buf, m_serializer->Serialization(request.data));

        return req_buf;
    }

    static RpcRequest RequestDecode(int sockfd) {
        // 解析服务名
        std::string svc = receive(sockfd);
        if (svc.empty()) return RpcRequest{};
        // 解析方法名称
        std::string mth = receive(sockfd);
        // 解析json_str
        std::string json_str = receive(sockfd);

        return RpcRequest{svc, mth, json::parse(json_str)};
    }

    static const std::string ResponseEncode(const RpcResponse &response) {
        std::string resp_buf;
        append_str(resp_buf, response.data.dump());
        return resp_buf;
    }

    static RpcResponse ResponseDecode(int sockfd) {
        // 读取长度
        rpc_len len;

        int bytes_read;
        if ((bytes_read = recv(sockfd, (char*)&len, sizeof(len), 0)) <= 0) {
            // 正常断开连接
            if (bytes_read == 0) {
                close(sockfd);
                throw std::runtime_error("Connection closed by server (EOF)");
            }

            if (errno != EINTR) {
                // 异常退出
                close(sockfd);
                throw std::runtime_error("Recv length error");
            }
        }

        // 读取json_str
        std::string json_str(len, '\0');
        if (recv(sockfd, (char*)json_str.data(), len, 0) <= 0) {
            close(sockfd);
            throw std::runtime_error("Recv data faild");
        }

        return RpcResponse{json::parse(json_str)};
    }
};

} // namespace minirpc