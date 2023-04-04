#pragma once
#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "./../common/util.hpp"
#include "../common/log.hpp"

// 只负责进行代码的编译

namespace ns_compiler
{
    using namespace ns_util;
    using namespace ns_log;

    class Compiler
    {
    public:
        // 返回值: 编译成功true，编译失败false
        // 输入参数: 编译的文件名（不带路径，不带后缀）
        static bool compile(const std::string &file_name)
        {
            // 子进程程序替换为g++，进行编译，成功则没有输出，可执行程序生成
            // 失败则g++会向标准错误中输出错误信息，即编译失败的原因
            pid_t pid = fork();
            if (pid < 0)
            {
                LOG(ERROR) << "内部错误，编译时创建子进程失败" << std::endl;
                return false;
            }
            else if (pid == 0)
            {
                // 子进程，进行程序替换g++，编译指定文件
                umask(0);
                int _compile_error_fd = open(PathUtil::CompileError(file_name).c_str(), O_CREAT | O_WRONLY, 0644);
                if (_compile_error_fd < 0)
                {
                    LOG(WARNING) << "内部错误, 编译时没有生成stderr文件" << std::endl;
                    exit(1); // 其实父进程不关心
                }
                // 程序替换，并不影响进程的文件描述符表
                dup2(_compile_error_fd, 2);
                execlp("g++", "g++", "-o", PathUtil::Exe(file_name).c_str(),
                       PathUtil::Src(file_name).c_str(), "-D", "COMPILE_ONLINE", "-std=c++11", nullptr);
                exit(2);  // 其实父进程不关心
            }
            else
            {
                // 父进程
                waitpid(pid, nullptr, 0); // 不关心子进程的退出结果，只关心是否编译成功（exe是否生成）
                // std::cout << "flag" << std::endl;
                if (FileUtil::IsFileExists(PathUtil::Exe(file_name)) == true)
                {
                    // 可执行程序已生成，编译成功
                    // std::cout << "flag 2" << std::endl;
                    LOG(INFO) << PathUtil::Src(file_name) << "编译成功!" << std::endl;
                    return true;
                }
            }
            // 可执行程序没有生成，g++错误信息已打印到对应的CompileError文件中。
            return false;
        }
    };
}