#include <iostream>
#include "mprpcapplication.h"
#include "user.pb.h"
#include "mprpcchannel.h"

int main(int argc, char **argv)
{
    // 初始化框架
    MprpcApplication::Init(argc, argv);
    // 调用方使用UserServiceRpc_Stub
    fixbug::UserServiceRpc_Stub stub(new MprpcChannel());
    // rpc请求参数
    fixbug::LoginRequest request;
    request.set_name("zhang san");
    request.set_pwd("123456");
    // rpc响应
    fixbug::LoginResponse response;
    // 发起rpc方法调用，调用到MprpcChannel::callmethod
    stub.Login(nullptr, &request, &response, nullptr);
    // 调用完成，读取调用结果
    if (response.result().errcode() == 0)
    {
        std::cout << "rpc login response success:" << response.success() << std::endl;
    }
    else
    {
        std::cout << "rpc login response err:" << response.result().errmsg() << std::endl;
    }
    // 演示调用远程发布的rpc方法Register
    fixbug::RegisterRequest req;
    req.set_id(2000);
    req.set_name("mprpc");
    req.set_pwd("666666");
    fixbug::RegisterResponse rsp;

    // 以同步的方式发起rpc调用请求，等待返回结果
    stub.Register(nullptr, &req, &rsp, nullptr);

    // 一次rpc调用完成，读调用的结果
    if (0 == rsp.result().errcode())
    {
        std::cout << "rpc register response success:" << rsp.sucess() << std::endl;
    }
    else
    {
        std::cout << "rpc register response error : " << rsp.result().errmsg() << std::endl;
    }
    return 0;
}