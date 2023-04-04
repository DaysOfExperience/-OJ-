#pragma once
#include <iostream>
#include <string>
#include <unordered_map>
#include <fstream>
#include <vector>
#include <cassert>
#include "../common/log.hpp"
#include "../common/util.hpp"

// 根据题目list文件，加载所有的题目信息到内存中(unordered_map)
// model: 主要用来和数据进行交互，对外提供访问数据的接口

namespace ns_model
{
    using namespace ns_log;
    using namespace ns_util;

    class Question
    {
    public:
        std::string number; // 题目编号
        std::string title;  // 题目标题
        std::string star;   // 题目难度
        int cpu_limit;      // 题目的时间要求(s)
        int mem_limit;      // 题目的空间要求(KB)
        std::string desc;   // 题目描述
        std::string header; // 内置代码
        std::string tail;   // 测试用例，和header拼接，形成完整可编译执行的代码
    };

    const std::string g_questions_list = "./questions/questions.list"; // questions_list文件的路径
    const std::string g_questions_path = "./questions/";

    class Model
    {
    private:
        std::unordered_map<std::string, Question> questions;
    public:
        Model()
        {
            assert(LoadAllQuestions(g_questions_list));
        }
        ~Model() {}
        bool LoadAllQuestions(const std::string &questions_list)
        {
            // 加载所有的题目数据到内存中(unordered_map)
            std::ifstream in(questions_list);
            if(!in.is_open())
            {
                LOG(FATAL) << "加载题库失败,请检查是否存在题目列表文件" << "\n";
                return false;
            }

            std::string line;
            while(std::getline(in, line))
            {
                // 获取到了一行数据
                // 1 两数之和 简单 1 30000
                std::vector<std::string> tokens;
                StringUtil::SplitString(line, &tokens, " ");
                if(tokens.size() != 5)
                {
                    // 这行数据无效
                    LOG(WARNING) << "加载部分题目失败,请检查文件格式" << "\n";
                    continue; // 继续加载下一个题目
                }
                Question q;
                q.number = tokens[0];
                q.title = tokens[1];
                q.star = tokens[2];
                q.cpu_limit = atoi(tokens[3].c_str());
                q.mem_limit = atoi(tokens[4].c_str());
                FileUtil::ReadFile(g_questions_path + tokens[0] + "/desc.txt", &q.desc, true);
                FileUtil::ReadFile(g_questions_path + tokens[0] + "/header.cpp", &q.header, true);
                FileUtil::ReadFile(g_questions_path + tokens[0] + "/tail.cpp", &q.tail, true);
                questions.insert({q.number, q});
            }
            LOG(INFO) << " 加载题库成功!" << "\n";
            in.close();
            return true;
        }
        // 提供获取所有题目数据和获取指定题目数据的接口
        bool GetAllQuestions(std::vector<Question> *out)
        {
            if(questions.size() == 0)
            {
                LOG(ERROR) << "用户获取题库失败" << "\n";
                return false;
            }
            for(const auto &iter : questions)
            {
                out->push_back(iter.second); // string : Question
            }
            return true;
        }
        bool GetOneQuestion(const std::string &number, Question *out)
        {
            if(questions.find(number) == questions.end())
            {
                LOG(ERROR) << "用户获取指定题目失败,题目编号: " << number << "\n";
                return false;
            }
            *out = questions[number];
            return true;
        }
    };
} // namespace ns_model