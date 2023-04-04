#pragma once
#include <iostream>
#include <string>
#include "../common/util.hpp"

namespace ns_log
{
    using namespace ns_util;

    // 日志等级，就是整数
    enum
    {
        INFO,
        DEBUG,
        WARNING,
        ERROR,
        FATAL
    };
    const char* levelDesc[] = 
    {
        "INFO",
        "DEBUG",
        "WARNING",
        "ERROR",
        "FATAL"
    };
    // LOG(WARNING) << "message" << "\n";
    // #level表示把level转换成字符串常量。例如，如果level是DEBUG，则#level将被替换为"DEBUG"。@1
    #define LOG(level) Log(level, __FILE__, __LINE__)
    // 开放式日志
    inline std::ostream &Log(int level, const std::string &file_name, int line)
    {
        // 日志等级
        std::string message = "[";
        message += levelDesc[level];
        message += "]";

        // 日志文件名称
        message += "[";
        message += file_name;
        message += "]";

        // 日志行
        message += "[";
        message += std::to_string(line);
        message += "]";

        // 日志时间戳
        message += "[";
        message += ns_util::TimeUtil::GetTimeStamp();
        message += "]";

        // std::cout内部是包含缓冲区的
        std::cout << message; // 不要endl进行刷新

        return std::cout;
    }
}