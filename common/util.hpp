#pragma once
#include <string>
#include <vector>
#include <atomic>
#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <boost/algorithm/string.hpp>

namespace ns_util
{
    class TimeUtil
    {
    public:
        static std::string GetTimeStamp() // 获取时间戳函数
        {
            // newwwww..
            struct timeval _time;
            gettimeofday(&_time, nullptr);
            return std::to_string(_time.tv_sec);
        }

        static std::string GetTimeMilliStamp()
        {
            struct timeval _time;
            gettimeofday(&_time, nullptr);
            return std::to_string(_time.tv_sec * 1000 + _time.tv_usec / 1000);
        }
    };
    const std::string temp_path = "./temp/";
    class PathUtil
    {
    public:
        // 编译时需要的临时文件
        // 编译时源文件路径+后缀名
        static std::string Src(const std::string &file_name)
        {
            return temp_path + file_name + ".cpp";
        }
        // 编译生成可执行程序的路径+后缀名
        static std::string Exe(const std::string &file_name)
        {
            return temp_path + file_name + ".exe";
        }
        // 编译失败时，存储g++的标准错误输出信息的路径+后缀名
        static std::string CompileError(const std::string &file_name)
        {
            return temp_path + file_name + ".compile_error";
        }
        // 运行时需要的临时文件
        // 运行时，标准输入重定向的路径+后缀名
        static std::string Stdin(const std::string &file_name)
        {
            return temp_path + file_name + ".stdin";
        }
        static std::string Stdout(const std::string &file_name)
        {
            return temp_path + file_name + ".stdout";
        }
        static std::string Stderror(const std::string &file_name)
        {
            return temp_path + file_name + ".stderror";
        }
    };

    class FileUtil
    {
    public:
        static bool IsFileExists(const std::string &path_name)
        {
            // newwwwww..
            struct stat _stat;
            if (stat(path_name.c_str(), &_stat) == 0)
            {
                // 获取属性成功，文件已存在
                return true;
            }
            return false;
        }
        // 生成唯一的文件名，不带路径和后缀，仅文件名
        static std::string UniqueFileName()
        {
            // 毫秒级时间戳 + 原子性递增唯一值 来保证文件名的唯一性
            static std::atomic_uint id(0); // 静态的！
            id++;
            std::string ms = TimeUtil::GetTimeMilliStamp();
            return ms + "_" + std::to_string(id);
        }
        // target: 写入文件的路径+后缀全名，content: 写入的内容
        static bool WriteFile(const std::string &target, const std::string &content)
        {
            // 将content内容写入到target中
            std::ofstream out(target);
            if (!out.is_open())
            {
                return false;
            }
            out.write(content.c_str(), content.size());
            out.close();
            return true;
        }
        // 读取target文件中的内容到content中(文件路径+文件名+后缀)
        // keep字段为是否保留文件中的\n字符，默认不保留
        static bool ReadFile(const std::string &target, std::string *content, bool keep = false)
        {
            // 读取文件内容到content中
            (*content).clear();

            std::ifstream in(target);
            if (!in.is_open())
            {
                return false;
            }
            std::string line;
            // 注意: getline不保留\n，但是有些时候我们需要\n（比如读取文件内容用于显示阅读）设置一个keep选项
            // getline内部重载了强制类型转换，转换为bool类型
            while (std::getline(in, line))
            {
                (*content) += line;
                (*content) += (keep ? "\n" : "");
            }
            in.close();
            return true;
        }
    };
    class StringUtil
    {
    public:
        static void SplitString(const std::string &str, std::vector<std::string> *target, const std::string &sep)
        {
            // 使用boost库的方法进行字符串切分
            //boost::split(*target, str, boost::is_any_of(sep), boost::algorithm::token_compress_on);  // sep的任意一个字符
            
            // 字符串切分:在str中找到sep,将切分结果放在target中
            //127.0.0.1:8080:45454:32323
            std::size_t index_left = 0;
            std::size_t index_right = 0;
            while(true)
            {
                index_right = str.find(sep, index_left);
                if(index_right == std::string::npos)
                {
                    // 没有找到
                    // std::cout << "index_left : " << index_left << std::endl;
                    if(index_left != str.size())
                    {
                        target->push_back(str.substr(index_left));
                    }
                    break;
                }
                target->push_back(str.substr(index_left, index_right));
                index_left = index_right + sep.size();
            }
        }
    };
}