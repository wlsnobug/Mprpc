#include"mprpcapplication.h"
#include<iostream>
#include <getopt.h>

MprpcConfig MprpcApplication::config_;
void ShowArgHelps()
{
    std::cout<<"fomat:command -i <configfile>"<<std::endl;
}

void MprpcApplication::Init(int argc,char **argv)
{
    if(argc<2)
    {
        ShowArgHelps();
        exit(EXIT_FAILURE);
    }
    int c = 0;
    std::string config_file;
    while((c = getopt(argc,argv,"i:"))!=-1)
    {
        switch (c)
        {
        case 'i':
            config_file = optarg;
            break;
        case '?':
            ShowArgHelps();
            exit(EXIT_FAILURE);
        case ':':
            ShowArgHelps();
            exit(EXIT_FAILURE);
        default:
            break;
        }
    }
    //开始加载配置文件
    config_.LoadConfigFile(config_file.c_str());
    // std::cout<<"rpcserverip:"<<config_.Load("rpcserverip")<<std::endl;
    // std::cout<<"rpcserverport:"<<config_.Load("rpcserverport")<<std::endl;
    // std::cout<<"zookeeperip:"<<config_.Load("zookeeperip")<<std::endl;
    // std::cout<<"zookeeperport:"<<config_.Load("zookeeperport")<<std::endl;
}
MprpcApplication& MprpcApplication::GetInstance()
{
    static MprpcApplication app;
    return app;
}

MprpcApplication::MprpcApplication()
{
}

MprpcConfig& MprpcApplication::GetConfig()
{
    return config_;
}