#include "logger.h"
#include "time.h"
#include<iostream>
// 获取日志的单例
Logger &Logger::GetInstance()
{
    static Logger logger;
    return logger;
}

Logger::Logger()
{
    // 启动专门的写日志线程
    std::thread writeLogTask([&]()->void{
        for(;;)
        {
            // 获取当前的日期，然后取日志信息，写入相应的日志文件当中 a+
            time_t now = time(nullptr);
            tm *nowtime = localtime(&now);
            char fileName[128];
            sprintf(fileName,"%d-%d-%d-log.txt",nowtime->tm_year+1900,nowtime->tm_mon+1,nowtime->tm_mday);
            FILE *pf = fopen(fileName,"a+");
            if(pf == nullptr)
            {
                std::cout<<"logger file : " << fileName << " open error!" << std::endl;
                exit(EXIT_FAILURE);
            }
            std::string msg = lockQue_.Pop();
            char timeBuf[128] = {0};
            sprintf(timeBuf,"%d:%d:%d =>[%s]",nowtime->tm_hour,nowtime->tm_min,nowtime->tm_sec,(logLevel_ == INFO ? "info" : "error"));
            msg.insert(0,timeBuf);
            msg.append("\n");
            fputs(msg.c_str(),pf);
            fclose(pf);
        }
    });
    writeLogTask.detach();
}
// 设置日志级别 
void Logger::SetLogLevel(LogLevel level)
{
    logLevel_ = level;
}
// 写日志， 把日志信息写入lockqueue缓冲区当中
void Logger::Log(std::string msg)
{
    lockQue_.Push(msg);
}