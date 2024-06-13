#include "test.pb.h"
#include <iostream>
#include <string>
using namespace fixbug;

int main()
{
    // LoginResponse rsp;
    // ResultCode*rc = rsp.mutable_result();
    // rc->set_errcode(0);
    // rc->set_errmsg("登录处理失败");
    GetFriendListResponse rsp;
    ResultCode *rc = rsp.mutable_result();
    rc->set_errcode(0);

    User *user1 = rsp.add_friend_list();
    user1->set_name("zhangsan");
    user1->set_age(20);
    user1->set_sex(User::MAN);

    User *user2 = rsp.add_friend_list();
    user2->set_name("lisi");
    user2->set_age(22);
    user2->set_sex(User::MAN);
    std::cout << rsp.friend_list_size() << std::endl;

    return 0;
}
int main1()
{
    LoginRequest req;
    req.set_name("zhang san");
    req.set_pwd("123456");

    // 数据对象序列化
    std::string send_str;

    if (req.SerializeToString(&send_str))
    {
        std::cout << send_str.c_str() << std::endl;
    }
    // 反序列化出一个数据对象
    LoginRequest reqBack;
    if (reqBack.ParseFromString(send_str))
    {
        std::cout << reqBack.name() << std::endl;
        std::cout << reqBack.pwd() << std::endl;
    }

    return 0;
}