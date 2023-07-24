// 需定制通信协议
#pragma once
#include "../compile_server/compiler.hpp"
#include "../compile_server/runner.hpp"
#include "../common/log.hpp"
#include "../common/util.hpp"
#include "jsoncpp/json/json.h"

namespace ns_compile_run
{
    using namespace ns_compiler;
    using namespace ns_runner;
    using namespace ns_log;
    using namespace ns_util;

    class CompileAndRun
    {
    public:
        /***************************************
         * 应用层协议定制：
         * 输入json串:
         * code： 用户提交的OJ代码
         * input: 用户提交的代码对应的输入(不做处理)
         * cpu_limit: OJ程序的时间要求
         * mem_limit: OJ程序的空间要求
         *
         * 输出json串:
         * 必填:
         * status: 状态码
         * reason: 请求结果（状态码描述）
         * 选填:（当OJ程序编译且运行成功时）
         * stdout: OJ程序运行完的标准输出结果
         * stderr: OJ程序运行完的标准错误结果
         * （若编译且运行成功out_json中才会有stdout和stderr字段）
         * 参数：
         * in_json: {"code": "#include...", "input": "","cpu_limit":1, "mem_limit":10240}
         * out_json: {"status":"0", "reason":"","stdout":"","stderr":"",}
         * ************************************/
        static void execute(const std::string &in_json, std::string *out_json)
        {
            // 输入的json串中有要编译的程序代码，对应的输入数据（忽略），时间空间限制等等
            // 提取出代码，写入到源文件中。
            // 编译源文件，看编译结果
            // 若编译成功，则运行可执行
            // 若一切顺利则status状态码为0，对应的输出结果也写入
            // 若某一步出现了错误，则status设置为对应的数字
            // reason也写好

            // 对json串进行反序列化，反序列化为结构化的数据
            Json::Value in_value;
            Json::Reader reader;
            reader.parse(in_json, in_value);

            // 提取出编译运行所需的数据
            std::string code = in_value["code"].asString();
            std::string input = in_value["input"].asString();
            int cpu_limit = in_value["cpu_limit"].asInt();
            int mem_limit = in_value["mem_limit"].asInt();

            int status = 0; // 状态码，最终要写入到out_json中
            std::string file_name;
            int run_result = 0;
            if (code.empty())
            {
                status = -1; // status=1: 用户输入的OJ代码为空(非服务端问题)
                goto END;
            }
            file_name = FileUtil::UniqueFileName();
            // 形成临时源文件，代码为传来的code，这个文件名不重要，唯一即可
            // 我们的目的是编译运行这份代码
            if (FileUtil::WriteFile(PathUtil::Src(file_name), code) == false)
            {
                status = -2; // status=2: 未知错误(服务端错误，具体为什么不管~（肯定是和写入文件有关联~）)
                goto END;
            }
            if (Compiler::compile(file_name) == false)
            {
                status = -3; // status=3: 编译失败(则不需要运行，也不能运行，因为没有可执行生成~)
                goto END;
            }
            run_result = Runner::run(file_name, cpu_limit, mem_limit);  // 可执行文件已经存在，传参文件名还有限制即可~
            if (run_result < 0)
            {
                status = -2; // 未知错误（不管run内部是打开文件失败还是创建子进程失败，统一称之为内部错误，即服务端错误）
            }
            else if (run_result > 0)
            {
                status = run_result; // 运行错误，程序崩溃，此时status为程序退出信号
            }
            else
            {
                status = 0; // 运行成功(且编译成功)
            }
        END:
            Json::Value out_value;
            out_value["status"] = status;                          // 状态码
            out_value["reason"] = StatusToDesc(status, file_name); // 状态码描述
            if (status == 0)  // 若运行错误>0，其他错误<0，则没有标准输出和标准错误字段在out_json中
            {
                // 只有当编译且运行成功时，才有stdout stderr字段
                std::string _stdout;
                FileUtil::ReadFile(PathUtil::Stdout(file_name), &_stdout, true); // 读取OJ程序的标准输出结果
                out_value["stdout"] = _stdout;
                std::string _stderr;
                FileUtil::ReadFile(PathUtil::Stderror(file_name), &_stderr, true); // 读取OJ程序的标准错误结果
                out_value["stderr"] = _stderr;
            }
            Json::StyledWriter writer;
            *out_json = writer.write(out_value); // 将out_value进行序列化
            RemoveTempFile(file_name);  // 清除编译运行生成的临时文件~
        }
        static std::string StatusToDesc(int status, const std::string &file_name)
        {
            // 将状态码转化为状态码描述
            std::string desc;
            switch (status)
            {
            case 0:
                desc = "编译且运行成功";
                break;
            case -1:
                desc = "用户输入的代码为空";
                break;
            case -2:
                desc = "未知错误"; // 服务端错误，太羞耻了
                break;
            case -3:
                // OJ代码编译时发生错误，状态码描述为编译错误原因（g++编译错误时的标准输出内容）
                FileUtil::ReadFile(PathUtil::CompileError(file_name), &desc, true);
                break;
            case SIGABRT: // 6
                desc = "程序占用内存资源超限";
                break;
            case SIGXCPU: // 24
                desc = "程序占用CPU资源超限";
                break;
            case SIGFPE: // 8
                desc = "浮点数溢出";
                break;
            default:
                desc = "未知: " + std::to_string(status);
                break;
            }
            return desc;
        }
        static void RemoveTempFile(const std::string &file_name)
        {
            // 有哪些文件生成不确定，但是文件名是有的，一共6个可能生成的临时文件，逐一检查。
            if (FileUtil::IsFileExists(PathUtil::Src(file_name)) == true)
                unlink(PathUtil::Src(file_name).c_str());

            if (FileUtil::IsFileExists(PathUtil::CompileError(file_name)) == true)
                unlink(PathUtil::CompileError(file_name).c_str());

            if (FileUtil::IsFileExists(PathUtil::Exe(file_name)) == true)
                unlink(PathUtil::Exe(file_name).c_str());

            if (FileUtil::IsFileExists(PathUtil::Stdin(file_name)) == true)
                unlink(PathUtil::Stdin(file_name).c_str());

            if (FileUtil::IsFileExists(PathUtil::Stdout(file_name)) == true)
                unlink(PathUtil::Stdout(file_name).c_str());

            if (FileUtil::IsFileExists(PathUtil::Stderror(file_name)) == true)
                unlink(PathUtil::Stderror(file_name).c_str());
        }
    };
}