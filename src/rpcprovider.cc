#include "rpcprovider.h"
#include "mprpcapplication.h"
#include "rpcheader.pb.h"
#include "logger.h"
#include "zookeeperutil.h"


// 框架提供的发布rpc方法的函数接口
void RpcProvider::NotifyService(google::protobuf::Service *service)
{
    ServiceInfo serviceInfo;
    // 获取了服务对象的描述信息
    const google::protobuf::ServiceDescriptor *pserviceDesc = service->GetDescriptor();
    // 获取服务的名字
    std::string serviceName = pserviceDesc->name();
    // 获取服务对象service的方法的数量
    int methodCount = pserviceDesc->method_count();
    LOG_INFO("service_name:%s", serviceName.c_str());
    for (int i = 0; i < methodCount; ++i)
    {
        // 获取了服务对象指定下标的服务方法的描述（抽象描述）
        const google::protobuf::MethodDescriptor *pmethodDesc = pserviceDesc->method(i);
        std::string methodName = pmethodDesc->name();
        serviceInfo.methodMap_.insert({methodName, pmethodDesc});
        LOG_INFO("method_name:%s", methodName.c_str());
    }
    serviceInfo.service_ = service;
    serviceMap_.insert({serviceName, serviceInfo});
}
// 启动rpc服务节点，开始提供rpc服务
void RpcProvider::run()
{
    // 读取配置文件rpcserver的信息
    std::string ip = MprpcApplication::GetInstance().GetConfig().Load("rpcserverip");
    uint16_t port = atoi(MprpcApplication::GetInstance().GetConfig().Load("rpcserverport").c_str());
    muduo::net::InetAddress address(ip, port);
    // 创建TcpServer对象
    muduo::net::TcpServer server(&eventloop_, address, "RpcProvider");
    // 绑定连接回调和消息读写回调方法  分离了网络代码和业务代码
    server.setConnectionCallback(std::bind(&RpcProvider::OnConnection, this, std::placeholders::_1));
    server.setMessageCallback(std::bind(&RpcProvider::OnMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    server.setThreadNum(4);
    // 把当前rpc节点上要发布的服务全部注册到zk上面，让rpc client可以从zk上发现服务
    // session timeout   30s     zkclient 网络I/O线程  1/3 * timeout 时间发送ping消息
    ZkClient zkCli;
    zkCli.Start();
    // service_name为永久性节点    method_name为临时性节点
    for(auto &sp : serviceMap_)
    {
        std::string servicePath = "/"+sp.first;
        zkCli.Create(servicePath.c_str(),nullptr,0);
        for(auto & mp : sp.second.methodMap_)
        {
            std::string methodPath = servicePath+"/"+mp.first;
            char methodPathData[128] = {0};
            sprintf(methodPathData,"%s:%d",ip.c_str(),port);
            // ZOO_EPHEMERAL表示znode是一个临时性节点
            zkCli.Create(methodPath.c_str(),methodPathData,strlen(methodPathData),ZOO_EPHEMERAL);
        }
    }
    LOG_INFO("RpcProvider start service at ip:%s port:%d",ip,port);
    // std::cout << "RpcProvider start service at ip:" << ip << " port:" << port << std::endl;
    // 启动网络服务
    server.start();
    eventloop_.loop();
}

void RpcProvider::OnConnection(const muduo::net::TcpConnectionPtr &conn)
{
    if (!conn->connected())
    {
        conn->shutdown();
    }
}
/*
在框架内部，RpcProvider和RpcConsumer协商好之间通信用的protobuf数据类型
serviceName methodName args    定义proto的message类型，进行数据头的序列化和反序列化
                                 serviceName methodName argsSize

headerSize(4个字节) + rpcHeaderStr + args
*/
void RpcProvider::OnMessage(const muduo::net::TcpConnectionPtr &conn,
                            muduo::net::Buffer *buffer,
                            muduo::Timestamp time)
{
    std::string recvBuffer = buffer->retrieveAllAsString();
    uint32_t headerSize = 0;
    recvBuffer.copy((char *)&headerSize, 4, 0);

    std::string rpcHeaderStr = recvBuffer.substr(4, headerSize);
    mprpc::RpcHeader rpcHeader;
    std::string serviceName;
    std::string methodName;
    uint32_t argsSize;
    if (rpcHeader.ParseFromString(rpcHeaderStr))
    {
        serviceName = rpcHeader.servicename();
        methodName = rpcHeader.methodname();
        argsSize = rpcHeader.argssize();
    }
    else
    {
        // 数据头反序列化失败
        LOG_ERR("rpcHeaderStr:%s parse error!",rpcHeaderStr);
        //std::cout << "rpcHeaderStr:" << rpcHeaderStr << " parse error!" << std::endl;
        return;
    }
    std::string args = recvBuffer.substr(4 + headerSize, argsSize);
    // 打印调试信息
    std::cout << "============================================" << std::endl;
    std::cout << "header_size: " << headerSize << std::endl;
    std::cout << "rpc_header_str: " << rpcHeaderStr << std::endl;
    std::cout << "service_name: " << serviceName << std::endl;
    std::cout << "method_name: " << methodName << std::endl;
    std::cout << "args_str: " << args << std::endl;
    std::cout << "============================================" << std::endl;
    auto it = serviceMap_.find(serviceName);
    if (it == serviceMap_.end())
    {
        LOG_ERR("%s is not exist!",serviceName);
        //std::cout << serviceName << " is not exist!" << std::endl;
        return;
    }
    auto mit = it->second.methodMap_.find(methodName);
    if (mit == it->second.methodMap_.end())
    {
        LOG_ERR("%s : %s is not exist!",serviceName,methodName);
        //std::cout << serviceName << ":" << methodName << " is not exist!" << std::endl;
        return;
    }
    google::protobuf::Service *service = it->second.service_;
    const google::protobuf::MethodDescriptor *method = mit->second;
    // 生成rpc方法调用的请求request和响应response参数
    google::protobuf::Message *request = service->GetRequestPrototype(method).New();
    if (!request->ParseFromString(args))
    {
        LOG_ERR("request parse error, content: %s",args);
        //std::cout << "request parse error, content:" << args << std::endl;
        return;
    }
    google::protobuf::Message *response = service->GetResponsePrototype(method).New();
    // 给下面的method方法的调用，绑定一个Closure的回调函数
    google::protobuf::Closure* done = google::protobuf::NewCallback<RpcProvider,const muduo::net::TcpConnectionPtr&, google::protobuf::Message*>
                                                                    (this, &RpcProvider::SendRpcResponse, conn, response);
    service->CallMethod(method,nullptr,request,response,done);
}

// Closure的回调操作，用于序列化rpc的响应和网络发送
void RpcProvider::SendRpcResponse(const muduo::net::TcpConnectionPtr &conn, google::protobuf::Message *response)
{
    std::string responseStr;
    if(response->SerializeToString(&responseStr))
    {
        // 序列化成功后，通过网络把rpc方法执行的结果发送会rpc的调用方
        conn->send(responseStr);
    }
    else
    {
        LOG_ERR("serialize response_str error!");
        //std::cout << "serialize response_str error!" << std::endl; 
    }
    conn->shutdown();
}