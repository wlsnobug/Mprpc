#pragma once
#include "google/protobuf/service.h"
#include <muduo/net/EventLoop.h>
#include <muduo/net/InetAddress.h>
#include <muduo/net/TcpServer.h>
#include <muduo/net/TcpConnection.h>
#include <google/protobuf/descriptor.h>

// 框架提供的发布rpc服务的网络对象类
class RpcProvider
{
public:
    // 框架提供的发布rpc方法的函数接口
    void NotifyService(google::protobuf::Service *service);
    // 启动rpc服务节点，开始提供rpc服务
    void run();

private:
    muduo::net::EventLoop eventloop_;
    //service服务类型信息
    struct ServiceInfo
    {
        google::protobuf::Service *service_;
        std::unordered_map<std::string,const google::protobuf::MethodDescriptor*>methodMap_;
    };
    // 存储注册成功的服务对象和其服务方法的所有信息
    std::unordered_map<std::string, ServiceInfo>serviceMap_;
    void OnConnection(const muduo::net::TcpConnectionPtr &conn);
    void OnMessage(const muduo::net::TcpConnectionPtr &conn,
                   muduo::net::Buffer *buffer,
                   muduo::Timestamp time);
    void SendRpcResponse(const muduo::net::TcpConnectionPtr &conn,google::protobuf::Message*response);
};