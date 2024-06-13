#include "mprpcchannel.h"
#include "rpcheader.pb.h"
#include <sys/types.h>
#include <sys/socket.h>
#include "mprpcapplication.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include "zookeeperutil.h"

// 所有通过stub代理对象调用的rpc方法，都走到这里了，统一做rpc方法调用的数据数据序列化和网络发送
void MprpcChannel::CallMethod(const google::protobuf::MethodDescriptor *method,
                              google::protobuf::RpcController *controller,
                              const google::protobuf::Message *request,
                              google::protobuf::Message *response,
                              google::protobuf::Closure *done)
{
    const google::protobuf::ServiceDescriptor *sd = method->service();
    std::string serviceName = sd->name();
    std::string methodName = method->name();
    // 获取参数的序列化字符串长度
    uint32_t argsSize = 0;
    std::string argsString;
    if (request->SerializeToString(&argsString))
    {
        argsSize = argsString.size();
    }
    else
    {
        controller->SetFailed("serialize request error!");
        return;
    }
    mprpc::RpcHeader rpcHeader;
    rpcHeader.set_servicename(serviceName);
    rpcHeader.set_methodname(methodName);
    rpcHeader.set_argssize(argsSize);
    uint32_t headerSize = 0;
    std::string rpcHeaderString;
    if (rpcHeader.SerializeToString(&rpcHeaderString))
    {
        headerSize = rpcHeaderString.size();
    }
    else
    {
        controller->SetFailed("serialize rpc header error!");
        return;
    }
    std::string sentString;
    sentString.insert(0, (char *)&headerSize, 4);
    sentString += rpcHeaderString;
    sentString += argsString;
    // 打印调试信息
    std::cout << "============================================" << std::endl;
    std::cout << "header_size: " << headerSize << std::endl; 
    std::cout << "rpc_header_str: " << rpcHeaderString << std::endl; 
    std::cout << "service_name: " << serviceName << std::endl; 
    std::cout << "method_name: " << methodName << std::endl; 
    std::cout << "args_str: " << argsString << std::endl; 
    std::cout << "============================================" << std::endl;
    int clientfd = socket(AF_INET,SOCK_STREAM,0);
    if(-1 == clientfd)
    {
        char errtxt[512] = {0};
        sprintf(errtxt, "create socket error! errno:%d", errno);
        controller->SetFailed(errtxt);
        return;
    }
    // 读取配置文件rpcserver的信息
    // std::string ip = MprpcApplication::GetInstance().GetConfig().Load("rpcserverip");
    // uint16_t port = atoi(MprpcApplication::GetInstance().GetConfig().Load("rpcserverport").c_str());
    // rpc调用方想调用service_name的method_name服务，需要查询zk上该服务所在的host信息
    ZkClient zkCli;
    zkCli.Start();
    //  /UserServiceRpc/Login
    std::string mathodPath = "/" + serviceName + "/" + methodName;
    // 127.0.0.1:8000
    std::string hostData = zkCli.GetData(mathodPath.c_str());
    if (hostData == "")
    {
        controller->SetFailed(mathodPath + " is not exist!");
        return;
    }
    int idx = hostData.find(":");
    if (idx == -1)
    {
        controller->SetFailed(mathodPath + " address is invalid!");
        return;
    }
    std::string ip = hostData.substr(0, idx);
    uint16_t port = atoi(hostData.substr(idx+1, hostData.size()-idx).c_str()); 
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = inet_addr(ip.c_str());
    //连接rpc服务节点
    if(-1 == connect(clientfd,(struct sockaddr *)&serverAddr,sizeof(serverAddr)))
    {
        close(clientfd);
        char errtxt[512] = {0};
        sprintf(errtxt, "connect error! errno:%d", errno);
        controller->SetFailed(errtxt);
        return;
    }
    //发送rpc请求
    if(-1 == send(clientfd,sentString.c_str(),sentString.size(),0))
    {
        close(clientfd);
        char errtxt[512] = {0};
        sprintf(errtxt, "send error! errno:%d", errno);
        controller->SetFailed(errtxt);
        return;
    }
    //接收rpc响应
    char recvBuf[1024] = {0};
    int recvSize = 0;
    if(-1 == (recvSize = recv(clientfd,recvBuf,1204,0)))
    {
        close(clientfd);
        char errtxt[512] = {0};
        sprintf(errtxt, "recv error! errno:%d", errno);
        controller->SetFailed(errtxt);
        return;
    }
    //反序列化rpc响应数据
    if(!response->ParseFromArray(recvBuf,recvSize))
    {
        close(clientfd);
        char errtxt[512] = {0};
        sprintf(errtxt,"parse error! response_str:%s", recvBuf);
        controller->SetFailed(errtxt);
        return;
    }
    close(clientfd);
}