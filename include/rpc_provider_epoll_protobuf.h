#pragma once

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <functional>
#include <map>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>

#include "log.h"
#include "nlohmann/json.hpp"
#include "protocol.h"
#include "constants.h"
#include "thread_pool/thread_pool.h"
#include "rpc_provider.h"
#include "rpc_provider_epoll.h"

#include "google/protobuf/service.h"
// --- 新增下面这两个头文件 ---
#include <google/protobuf/descriptor.h> // 提供 ServiceDescriptor, MethodDescriptor 的完整定义
#include <google/protobuf/message.h>    // 提供 Message 的完整定义 (ParseFromString, New 等)
#include "google/protobuf/stubs/callback.h"

using json = nlohmann::json;
#ifndef MAX_EVENTS
#define MAX_EVENTS 1024
#endif

// rpc 服务提供

namespace minirpc
{
class RpcProviderEpollProtobuf
{
private:
    /* data */
    std::map<std::string, RpcHandler> m_handlers;
    int _port;
    ThreadPool m_pool;
    std::map<std::string, google::protobuf::Service*> service_map_;

public:
    RpcProviderEpollProtobuf(int port = 8080);
    ~RpcProviderEpollProtobuf();

private:
    void OnMessage(const int sockfd) const;
    void OnConnect(const int epfd, const int sockfd) const;
    void OnMessageEvent(const int epfd, const int socifd) const;
    void OnMessageEventProtobuf(const int epfd, const int sockfd);

public:
    // 注册方法
    void RegisterMethod(const std::string &svc, const std::string mth, const RpcHandler &handler);
    void RegisterMethod(const std::string &svc, google::protobuf::Service* service);

    // 服务启动，对外开放连接处理请求,进入阻塞状态
    void Run();

    void EpollRun();
    void SendResponse(int sockfd, google::protobuf::Message* response);
};

RpcProviderEpollProtobuf::RpcProviderEpollProtobuf(int port) : _port(port)
{
}

RpcProviderEpollProtobuf::~RpcProviderEpollProtobuf()
{
}

// 将服务函数进行注册
void RpcProviderEpollProtobuf::RegisterMethod(const std::string &svc, const std::string mth, const RpcHandler &handler) {
    std::string key = svc + "." + mth;
    m_handlers[key] = handler;

    LOG_INFO("Register method %s", key.c_str());
}

void RpcProviderEpollProtobuf::RegisterMethod(const std::string &svc, google::protobuf::Service* service) {
    service_map_[svc] = service;

    LOG_INFO("Register service %s", svc.c_str());
}

void RpcProviderEpollProtobuf::OnConnect(const int epfd, const int sockfd) const {
    struct sockaddr_in client_addr;
    socklen_t sock_len = sizeof(client_addr);

    // 循环遍历accept
    while (true) {
        int client_fd = accept(sockfd, (struct sockaddr*)&client_addr, &sock_len);

        if (client_fd == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break; // 没有更多连接了
            }

            perror("accept error");
            exit(1);
        }

        // 打印当前连接信息
        std::cout << "[Epoll] New client connected " <<
        inet_ntoa(client_addr.sin_addr) << ":" << ntohs(client_addr.sin_port) << " (fd = " << client_fd << ")" << std::endl;

        // 关键：TODO: 将新连接设置为非阻塞
        set_nonblocking(client_fd);
        
        epoll_event event;
        // 注册新连接到epoll
        event.events = EPOLLIN;
        event.data.fd = client_fd;

        if (epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &event) == -1) {
            perror("epoll_ctl error");
            close(client_fd);
        }

    }
}

// 处理用户请求
void RpcProviderEpollProtobuf::OnMessage(const int sockfd) const {
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


void RpcProviderEpollProtobuf::Run() {
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


void RpcProviderEpollProtobuf::EpollRun() {
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
    if (listen(sockfd, 1024) == -1) { // listen 第二个参数的含义是当连接数小于设置值时会进入等待队列，若超过该值，则直接拒绝服务请求
        perror("listen error");
        close(sockfd);
        exit(1);
    }

    // 设置为非阻塞
    set_nonblocking(sockfd);

    std::cout << "============== Epoll TCP Server (LT MODE)==============" << std::endl;
    std::cout << "listen net port: " << _port << std::endl;

    
    // 5. 创建epoll实例
    int epfd = epoll_create(1);

    if (epfd == -1) {
        perror("epoll create error");
        close(sockfd);
        exit(1);
    }

    // 6. 注册监听Socket到epoll
    epoll_event event;
    event.events = EPOLLIN; // 关注可读事件
    event.data.fd = sockfd; // 保存fd, 方便后续取出

    // 注册
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &event) == -1) {
        perror("epoll_ctl error");
        close(sockfd);
        close(epfd);
        exit(1);
    }

    // 7. 事件循环
    epoll_event events[MAX_EVENTS];

    while (true) {
        // 阻塞等待
        // 返回待处理的事件数量
        int n = epoll_wait(epfd, events, MAX_EVENTS, -1); // -1 表示无限阻塞等待

        if (n == -1) {
            if (errno == EINTR) continue; // 被信号中断继续
            perror("epoll_wait error");
            break;
        }

        // 遍历所有的就绪事件
        for (int i = 0; i < n; i++)
        {
            int c_fd = events[i].data.fd;
            // 情况A: 监听socket事件（有新连接）
            if (c_fd == sockfd) {
                OnConnect(epfd, sockfd);
            }
            else { // 情况B: 客户端Socket fd 就绪（可读或断开）
                // OnMessageEvent(epfd, c_fd);
                OnMessageEventProtobuf(epfd, c_fd);
            }
        }
        
    }


    // 关闭socket
    close(sockfd);
    close(epfd);
}


void RpcProviderEpollProtobuf::OnMessageEvent(const int epfd, const int sockfd) const {
    LOG_INFO("Epoll Event Trigger");
    char buffer[BUFFER_SIZE];

    while (true) { // 循环读取
        memset(buffer, 0, BUFFER_SIZE);

        RpcRequest request;
        // 接收数据并解码，可能需要进行catch，用来捕获异常或断开连接
        try
        {
            request = Protocol::RequestDecode(sockfd);
        }
        catch(const std::exception& e)
        {
            close(sockfd);
            epoll_ctl(epfd, EPOLL_CTL_DEL, sockfd, NULL); // 移除fd
            break;
        }
        
        // 处理errno == EINTR情况
        if (request.serviceName.empty()) {
            break;
        }

        // 无异常，进行函数调用，并返回结果
        json response;
        std::string key = request.serviceName + '.' + request.methodName;

        // 调用方法进行处理
        if (m_handlers.find(key) != m_handlers.end()) {
            m_handlers.at(key)(request.data, response);

            // 将结果返回到客户端
            if (!Protocol::SendResponse(sockfd, RpcResponse{response})) {
                // 发送异常
                close(sockfd);
                epoll_ctl(epfd, EPOLL_CTL_DEL, sockfd, NULL); // 移除fd
                break;
            }
        }
        else {
            LOG_ERROR("Method Not found %s", key.c_str());
            if (!Protocol::SendResponse(sockfd, METHOD_NOT_FUND)) {
                // 发送异常
                close(sockfd);
                epoll_ctl(epfd, EPOLL_CTL_DEL, sockfd, NULL); // 移除fd
                break;
            }
            continue;
        }

        break;
    }
}


void RpcProviderEpollProtobuf::OnMessageEventProtobuf(const int epfd, const int sockfd) {
    LOG_INFO("OnMessageProtobuf()");
    // 读取数据， 假如一次读取完成
    char buffer[BUFFER_SIZE];

    while (true) {
        memset(buffer, 0, BUFFER_SIZE); // 重置缓冲区        

        // 接收数据
        std::vector<std::string>  msgs;
        try
        {
            msgs = Protocol::ReceiveDecode(sockfd);
        }
        catch(const std::exception& e)
        {
            // 异常或者断开退出, 关闭监听
            epoll_ctl(epfd, EPOLL_CTL_DEL, sockfd, nullptr);
            break;
        }

        std::string &svc = msgs[0];
        std::string &mth = msgs[1];
        std::string &req = msgs[2];

        if (svc.empty()) break;

        std::string key = svc + "." + mth;


        // --- 第一步：查找服务 ---
        if (service_map_.find(svc) == service_map_.end()) {
            std::cerr << "Error: Service not found: " << svc << std::endl;
            return;
        }
        google::protobuf::Service* service = service_map_[svc];

        // --- 第二步：获取方法描述符 (Metadata) ---
        const google::protobuf::ServiceDescriptor* sd = service->GetDescriptor();
        const google::protobuf::MethodDescriptor* md = sd->FindMethodByName(mth);

        if (!md) {
            std::cerr << "Error: Method not found: " << mth << std::endl;
            return;
        }

        // --- 第三步：动态创建 Request 和 Response 对象 ---
        // 关键点：框架不需要知道具体是 LoginRequest 还是 GetUserInfoRequest
        // 它通过描述符问服务：“这个方法的输入/输出类型是什么？”然后创建空对象
        google::protobuf::Message* request = service->GetRequestPrototype(md).New();
        google::protobuf::Message* response = service->GetResponsePrototype(md).New();

        // --- 第四步：反序列化 (String -> Object) ---
        // 把接收到的二进制字符串填入 request 对象
        if (!request->ParseFromString(req)) {
            std::cerr << "Error: Failed to parse request data." << std::endl;
            delete request;
            delete response;
            return;
        }
        // 此时 request 内部已经填满了数据 (如 name="zhangsan")，只是类型是基类指针

        // 5. 【修改点】创建回调
        // 语法：NewCallback(对象指针, &成员函数, 参数1, 参数2, ...)
        google::protobuf::Closure* done = google::protobuf::NewCallback(
            this,                   // 对象指针 (当前 RpcProvider 实例)
            &RpcProviderEpollProtobuf::SendResponse, // 成员函数指针
            sockfd,                 // 传递给 SendResponse 的参数 1
            response                // 传递给 SendResponse 的参数 2
        );

        // --- 第六步：反射调用 (核心!) ---
        // 框架把“任务”丢给服务对象，让服务对象自己去执行具体的 Login 或 GetUserInfo
        // controller 传 nullptr 即可
        service->CallMethod(md, nullptr, request, response, done);

        // --- 第七步：清理 Request ---
        // request 的生命周期结束，可以删除了
        // (注意：response 不能在删，因为它被 done 回调持有了，会在发送完后由 done 删除)
        delete request;
    }
}

void RpcProviderEpollProtobuf::SendResponse(int sockfd, google::protobuf::Message* response) {
    std::string resp_str;
    // 序列化响应
    if (response->SerializeToString(&resp_str)) {
        const std::string bytes = Protocol::Encode({resp_str});
        rpc_len len = bytes.size();
        if (send(sockfd, bytes.data(), len, 0) < 0) {
            std::cerr << "Failed to send response length." << std::endl;
        }

        // // 构造协议：[长度][数据]
        // uint32_t len = resp_str.size();
        
        // // 发送长度
        // if (send(sockfd, (char*)&len, sizeof(len), 0) < 0) {
        //     std::cerr << "Failed to send response length." << std::endl;
        // }
        // // 发送数据
        // else if (send(sockfd, resp_str.c_str(), len, 0) < 0) {
        //     std::cerr << "Failed to send response data." << std::endl;
        // }
    } else {
        std::cerr << "Failed to serialize response message." << std::endl;
    }

    // 【重要】清理内存
    // 这个 response 是在 HandleRequest 里 new 出来的，必须由我们 delete
    delete response;
}

} // namespace minirpc
