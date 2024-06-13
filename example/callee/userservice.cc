#include <iostream>
#include <string>
#include "user.pb.h"
#include "mprpcapplication.h"
#include "rpcprovider.h"

class UserService : public fixbug::UserServiceRpc
{
public:
    // 本地业务函数
    bool Login(std::string name, std::string pwd)
    {
        std::cout << "doing local service Login" << std::endl;
        std::cout << "name:" << name << "pwd:" << pwd << std::endl;
        return true;
    }

    bool Register(uint32_t id, std::string name, std::string pwd)
    {
        std::cout << "doing local service: Register" << std::endl;
        std::cout << "id:" << id << "name:" << name << " pwd:" << pwd << std::endl;
        return true;
    }

    // 重写基类UserServiceRpc中生成的Login虚函数，框架调用该虚函数发生多态
    // 我方是分布式应用的发布者callee，发布Login函数提供分布式服务：1、定义好proto文件2、重写基类中的同名函数
    // caller请求Login(LoginRequest),通过muduo传送到callee
    // callee获取框架传回的Login(LoginRequest),交给下面的函数进行业务处理
    void Login(::google::protobuf::RpcController *controller,
               const ::fixbug::LoginRequest *request,
               ::fixbug::LoginResponse *response,
               ::google::protobuf::Closure *done)
    {
        // 从框架获取caller提供的数据
        std::string name = request->name();
        std::string pwd = request->pwd();
        // 使用数据做本地业务
        bool loginResult = Login(name, pwd);
        // 将处理结果写入回传消息response，由框架负责回传
        fixbug::ResultCode *code = response->mutable_result();
        code->set_errcode(0);
        code->set_errmsg("");
        response->set_success(loginResult);
        // 执行回传操作
        done->Run();
    }
    void Register(::google::protobuf::RpcController *controller,
                  const ::fixbug::RegisterRequest *request,
                  ::fixbug::RegisterResponse *response,
                  ::google::protobuf::Closure *done)
    {
        uint32_t id = request->id();
        std::string name = request->name();
        std::string pwd = request->pwd();
        bool ret = Register(id,name,pwd);
        response->mutable_result()->set_errcode(0);
        response->mutable_result()->set_errmsg("");
        response->set_sucess(ret);
        done->Run();
    }
};

int main(int argc, char **argv)
{
    MprpcApplication::Init(argc, argv);
    RpcProvider provider;
    provider.NotifyService(new UserService());
    provider.run();
    return 0;
}