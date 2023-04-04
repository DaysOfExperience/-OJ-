#pragma once
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <fcntl.h>
#include "../common/util.hpp"
#include "../common/log.hpp"

namespace ns_runner
{
    using namespace ns_util;
    using namespace ns_log;

    // 只负责运行编译好的可执行
    class Runner
    {
    public:
        /* 返回值 > 0，oj程序运行异常，收到了信号，返回值为信号编号
         * 返回值 = 0，oj程序运行成功，标准输出和标准错误信息在对应的文件中
         * 返回值 < 0，内部错误。如打开文件失败，创建子进程执行oj程序时失败。
         * cpu_limit: file_name程序运行时，可以使用的CPU资源上限(时间，秒)
         * mem_limit: file_name程序运行时，可以使用的内存资源上限(KB)
         */
        static int run(const std::string file_name, int cpu_limit, int mem_limit)
        {
            std::string _excute = PathUtil::Exe(file_name);
            std::string _stdin = PathUtil::Stdin(file_name);
            std::string _stdout = PathUtil::Stdout(file_name);
            std::string _stderror = PathUtil::Stderror(file_name);
            // 运行程序，程序的输入，输出，错误信息进行重定向的文件
            umask(0);
            int _stdin_fd = open(_stdin.c_str(), O_CREAT | O_RDONLY, 0644);   // 不处理，便于扩展
            int _stdout_fd = open(_stdout.c_str(), O_CREAT | O_WRONLY, 0644);   // OJ程序的输出结果
            int _stderror_fd = open(_stderror.c_str(), O_CREAT | O_WRONLY, 0644); // OJ程序的运行时错误信息
            if (_stdin_fd < 0 || _stdout_fd < 0 || _stderror_fd < 0)
            {
                LOG(ERROR) << "内部错误，运行时打开文件失败" << std::endl;
                return -1; // 代表打开文件失败
            }

            pid_t pid = fork();
            if (pid < 0)
            {
                LOG(ERROR) << "运行时创建子进程失败" << std::endl;
                close(_stdin_fd);
                close(_stdout_fd);
                close(_stderror_fd);
                return -2; // 代表创建子进程失败
            }
            else if (pid == 0)
            {
                // 子进程进行程序替换，执行可执行程序
                dup2(_stdin_fd, 0);
                dup2(_stdout_fd, 1);
                dup2(_stderror_fd, 2);
                SetProcLimit(cpu_limit, mem_limit);
                execl(_excute.c_str(), _excute.c_str(), nullptr);
                exit(1);
            }
            else
            {
                close(_stdin_fd);
                close(_stdout_fd);
                close(_stderror_fd);
                // 父进程获知程序的执行情况，仅关心成功执行or异常终止
                // 对于成功执行之后的执行结果并不关心，是上层的任务，需要根据测试用例判断
                int status = 0;
                waitpid(pid, &status, 0);
                LOG(INFO) << "OJ题运行完毕, 退出信号: " << (status & 0x7F) << std::endl;
                return status & 0x7F; // 将子进程的退出信号返回（并非退出码）
            }
        }
        // 设置进程占用资源大小的接口（CPU资源，内存资源）mem_limit的单位为KB
        static void SetProcLimit(int _cpu_limit, int _mem_limit)
        {
            // 设置进程占用CPU时长限制
            struct rlimit cpu_rlimit;
            cpu_rlimit.rlim_max = RLIM_INFINITY;
            cpu_rlimit.rlim_cur = _cpu_limit;
            setrlimit(RLIMIT_CPU, &cpu_rlimit);

            // 设置进程占用内存资源限制
            struct rlimit mem_limit;
            mem_limit.rlim_max = RLIM_INFINITY;
            mem_limit.rlim_cur = _mem_limit * 1024; // 转化为KB
            setrlimit(RLIMIT_AS, &mem_limit);
        }
    };
}