#pragma once

#include "user.pb.h"
#include <google/protobuf/service.h>
#include <google/protobuf/message.h> // 确保包含这个
#include <cassert>

namespace fixbug
{

// 手动定义抽象基类 (模仿 gRPC 生成的结构)
class UserService : public google::protobuf::Service
{
private:
    // 用于获取原型的工厂类 (单例或成员均可)
    google::protobuf::MessageFactory* factory_ = google::protobuf::MessageFactory::generated_factory();

public:
    virtual ~UserService() {};

    // 声明业务方法 模拟gRPC 生成
    virtual void Login(
        google::protobuf::RpcController *controller,
        const LoginRequest *request,
        LoginResponse * response,
        google::protobuf::Closure *closure) const = 0;

    // 声明业务方法
    virtual void GetUserInfo(
        google::protobuf::RpcController *controller,
        const GetUserInfoRequest *request,
        GetUserInfoResponse * response,
        google::protobuf::Closure *closure) const = 0;

    // --- 必须实现的 google::protobuf::Service 接口 (用于反射) ---
    // 为了简化，我们可以让编译器帮我们生成一部分，或者简单实现
    // 但最简单的做法是：直接让具体实现类继承这个类，并在 RpcProvider 里特化调用
    
    // 注意：如果要完美兼容 protobuf 的 CallMethod，需要实现下面这些
    // 但为了学习，我们可以绕过 CallMethod，直接用模板特化调用具体方法


    // --- 必须实现的 Service 接口 (用于反射) ---
    // 这些方法让框架能通过字符串名字找到并调用上面的业务方法

    // 静态方法：获取该服务的描述符
    static const google::protobuf::ServiceDescriptor* descriptor() {
        return LoginRequest::descriptor()->file()->FindServiceByName("UserService");
    }

    
    // 获取服务描述符 (从生成的 code 中获取)
    // 1. 获取服务描述符
    const google::protobuf::ServiceDescriptor* GetDescriptor() override {
        return descriptor();
    }

    // 2. 【修复】实现 GetRequestPrototype
    const google::protobuf::Message& GetRequestPrototype(
        const google::protobuf::MethodDescriptor* method) const override {
        
        // 通过方法描述符获取输入类型，再获取原型
        return *factory_->GetPrototype(method->input_type());
    }

    // 3. 【修复】实现 GetResponsePrototype
    const google::protobuf::Message& GetResponsePrototype(
        const google::protobuf::MethodDescriptor* method) const override {
        
        // 通过方法描述符获取输出类型，再获取原型
        return *factory_->GetPrototype(method->output_type());
    }

    // CallMethod: 框架入口，根据 MethodDescriptor 分发到具体函数
    // CallMethod: 框架入口
    // ⚠️ 注意：这里调用的 Login 是 const 的，所以没问题。
    // 但如果你的业务逻辑需要修改成员变量，Login 就不能是 const。
    void CallMethod(
        const google::protobuf::MethodDescriptor* method,
        google::protobuf::RpcController* controller,
        const google::protobuf::Message* request,
        google::protobuf::Message* response,
        google::protobuf::Closure* done) override {

        if (method->name() == "Login") {
            // 类型检查
            if (request->GetTypeName() != "fixbug.LoginRequest" || 
                response->GetTypeName() != "fixbug.LoginResponse") {
                if(done) done->Run();
                return;
            }

            Login(controller, 
                static_cast<const fixbug::LoginRequest*>(request),
                static_cast<fixbug::LoginResponse*>(response),
                done);
        } 
        else if (method->name() == "GetUserInfo") {
            if (request->GetTypeName() != "fixbug.GetUserInfoRequest" || 
                response->GetTypeName() != "fixbug.GetUserInfoResponse") {
                if(done) done->Run();
                return;
            }

            GetUserInfo(controller,
                        static_cast<const fixbug::GetUserInfoRequest*>(request),
                        static_cast<fixbug::GetUserInfoResponse*>(response),
                        done);
        } 
        else {
            if (done) done->Run();
        }
    }
};
    
} // namespace fixbug