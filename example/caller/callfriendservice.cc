#include <iostream>
#include "mprpcapplication.h"
#include "friend.pb.h"

int main(int argc, char **argv)
{
    // 初始化框架
    MprpcApplication::Init(argc, argv);
    // 调用方使用FriendServiceRpc_Stub
    fixbug::FriendServiceRpc_Stub stub(new MprpcChannel());
    // rpc请求参数
    fixbug::GetFriendsListRequest request;
    request.set_userid(1000);
    // rpc响应
    fixbug::GetFriendsListResponse response;
    MprpcController controller;
    // 发起rpc方法调用，调用到MprpcChannel::callmethod
    stub.GetFriendsList(nullptr, &request, &response, nullptr);
    // 调用完成，读取调用结果
    if (controller.Failed())
    {
        std::cout << controller.ErrorText() << std::endl;
    }
    else
    {
        if (0 == response.result().errcode())
        {
            std::cout << "rpc GetFriendsList response success!" << std::endl;
            int size = response.friends_size();
            for (int i=0; i < size; ++i)
            {
                std::cout << "index:" << (i+1) << " name:" << response.friends(i) << std::endl;
            }
        }
        else
        {
            std::cout << "rpc GetFriendsList response error : " << response.result().errmsg() << std::endl;
        }
    }

    return 0;
}