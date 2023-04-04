#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <mutex>
#include <fstream>
#include <cassert>
#include <jsoncpp/json/json.h>
#include "./oj_model.hpp"
#include "./oj_view.hpp"
#include "../common/httplib.h"

namespace ns_control
{
    using namespace ns_model;
    using namespace ns_view;
    using namespace httplib;

    // 为什么要加锁??????aaaa
    class Machine
    {
    public:
        std::string ip_;
        int port_;
        uint64_t load_;    // 该机器的负载
        std::mutex *mtx_;  // std::mutex不可被拷贝，所以Machine将不能被拷贝，故存储指针,使Machine可以被拷贝(存入容器中)
    public:
        Machine()
        : ip_(), port_(0), load_(0), mtx_(nullptr)
        {}
        ~Machine()
        {
            // if(mtx_ != nullptr)
            //     delete mtx_;
        }
        void IncLoad()
        {
            mtx_->lock();
            ++load_;
            mtx_->unlock();
        }
        void DecLoad()
        {
            mtx_->lock();
            --load_;
            mtx_->unlock();
        }
        void ResetLoad()
        {
            mtx_->lock();
            load_ = 0;
            mtx_->unlock();
        }
        uint64_t Load()
        {
            mtx_->lock();
            uint64_t load = load_;
            mtx_->unlock();
            return load;
        }
    };
    
    const std::string g_file_path = "./conf/service_machine.conf";

    // 负载均衡模块
    class LoadBalance
    {
    private:
        std::vector<Machine> machines_;   // 所有的机器
        std::vector<int> online_;         // 在线机器,存储所有在线机器的下标
        std::vector<int> offline_;        // 离线机器,存储所有离线机器的下标
        std::mutex mtx_;
    public:
        LoadBalance()
        {
            assert(LoadAllMachines(g_file_path));
            LOG(INFO) << " 加载所有编译运行服务器成功!" << "\n";
        }
        ~LoadBalance() {}
        bool LoadAllMachines(const std::string& file_path)
        {
            // 将配置文件中的所有后端机器加载到machines和online中
            std::ifstream in(file_path);
            if(!in.is_open())
            {
                LOG(FATAL) << "加载 " << file_path << " 失败" << "\n";
                return false;
            }
            // std::cout << "debug 1" << std::endl;
            std::string line;
            while(std::getline(in, line))
            {
                // 127.0.0.1:8081
                
                std::vector<std::string> tokens;
                StringUtil::SplitString(line, &tokens, ":");
                if(tokens.size() != 2)
                {
                    LOG(WARNING) << "切分" << line << "失败" << "\n";
                    continue;
                }

                // for(auto &i:tokens)
                // {
                //     std::cout << i << std::endl;
                // }
                Machine m;
                m.ip_ = tokens[0];
                m.port_ = atoi(tokens[1].c_str());
                m.load_ = 0;
                m.mtx_ = new std::mutex();

                online_.push_back(machines_.size());
                machines_.push_back(m);
            }

            in.close();

            return true;
        }
        bool SmartChoice(int *id, Machine **m)
        {
            // 进行智能选择
            mtx_.lock();
            int online_num = online_.size();
            if(0 == online_num)
            {
                mtx_.unlock();
                LOG(FATAL) << "所有的后端编译服务器都已离线,请运维同事尽快处理" << "\n";
                return false;
            }
            // 至少有一台可以提供编译服务的服务器
            *id = online_[0];
            for(int i = 1; i < online_.size()/*online_num*/; ++i)
            {
                if(machines_[online_[i]].load_ < machines_[*id].load_)
                {
                    *id = online_[i];
                }
            }
            *m = &machines_[*id];  // 获取一个一级指针，传一个二级指针
            mtx_.unlock();
            return true;
        }
        void OfflineMachine(int id)
        {
            // 离线主机
            mtx_.lock();
            for(auto iter = online_.begin(); iter != online_.end(); ++iter)
            {
                if(*iter == id)
                {
                    machines_[id].ResetLoad();
                    offline_.push_back(id);
                    online_.erase(iter);
                    break;   // 因为break，所以此处不会出现vector的迭代器失效问题。
                }
            }
            // offline_.push_back(id);   // 有问题，不能在这里。
            mtx_.unlock();
        }
        void OnlineMachine()
        {
            mtx_.lock();
            online_.insert(online_.begin(), offline_.begin(), offline_.end());
            offline_.erase(offline_.begin(), offline_.end());
            mtx_.unlock();
            LOG(INFO) << "所有的主机已上线" << "\n";
        }
        // 用于调试
        void ShowMachines()
        {
            mtx_.lock();
            std::cout << "当前在线主机id列表 : ";
            for(auto &i :online_)
                std::cout << i << " ";
            std::cout << std::endl;
            std::cout << "当前离线主机id列表 : ";
            for(auto &i : offline_)
                std::cout << i << " ";
            std::cout << std::endl;
            mtx_.unlock();
        }
    };

    class Control
    {
    private:
        Model model_;
        View view_;
        LoadBalance loadbalance_;
    public:
        bool AllQuestions(std::string *html)
        {
            bool ret = true;
            std::vector<Question> questions;
            if(model_.GetAllQuestions(&questions))
            {
                // std::cout << "debug : " << questions[0].title << std::endl;
                // 获取题目列表数据成功,将所有的题目数据渲染构建成网页
                view_.AllExpandHtml(questions, html);
            }
            else
            {
                *html = "获取题目列表失败";
                ret = false;
            }
            return ret;
        }
        bool OneQuestion(const std::string &number, std::string *html)
        {
            bool ret = true;
            Question q;
            if(model_.GetOneQuestion(number, &q))
            {
                // 获取指定题目信息成功, 将题目的所有数据渲染构建成网页
                view_.OneExpandHtml(q, html);
            }
            else
            {
                *html = "获取指定题目失败,题目编号: " + number;
                ret = false;
            }
            return ret;
        }
        void Judge(const std::string &number, const std::string in_json, std::string *out_json)
        {
            // 1. 获取题目信息
            Question q;
            model_.GetOneQuestion(number, &q);

            // 2. 反序列化,从in_json中获取到code和input
            Json::Value in_value;
            Json::Reader reader;
            reader.parse(in_json, in_value);   // 反序列化
            std::string code = in_value["code"].asString();
            std::string input = in_value["input"].asString();

            // 3. 构造给编译模块的json串
            Json::Value compile_value;
            compile_value["code"] = code + q.tail;   // 用户提交的代码 + 测试用例 = 最终进行编译运行的代码
            compile_value["input"] = input;
            compile_value["cpu_limit"] = q.cpu_limit;
            compile_value["mem_limit"] = q.mem_limit;
            Json::FastWriter writer;
            std::string compile_json = writer.write(compile_value);

            // 4. 负载均衡地选择一个后端提供编译运行服务的服务器，获取编译服务，获取编译运行结果传给out_json
            while(true)
            {
                int id = 0;
                Machine *m = nullptr;
                if(!loadbalance_.SmartChoice(&id, &m))
                {
                    // 选取编译服务器失败,即所有的后端编译服务器都已离线
                    break;
                }
                // 选取到了一个编译服务器
                Client cli(m->ip_, m->port_);
                m->IncLoad();   // 增加对应编译服务器负载
                LOG(ERROR) << " 选择编译主机成功,主机id : " << id << " 详情 : " << m->ip_ << ":" << m->port_ << " 当前主机负载为 : " << m->Load() << "\n";
                if(auto res = cli.Post("/compile_and_run", compile_json, "application/json;charset=utf-8"))
                {
                    if(res->status == 200)
                    {
                        // 请求成功，且状态码为200
                        m->DecLoad();
                        *out_json = res->body;  // 编译运行的结果
                        LOG(INFO) << "请求编译和运行服务成功" << "\n";
                        break;
                    }
                    // 请求成功，但是退出码不为200，故需要再次选择编译服务器&&再次请求
                    m->DecLoad();
                }
                else
                {
                    // 请求失败，直接把该主机挂掉即可
                    LOG(ERROR) << " 当前请求的主机 : " << id << " 详情 : " << m->ip_ << ":" << m->port_ << " 可能已离线" << "\n";
                    loadbalance_.OfflineMachine(id);
                    loadbalance_.ShowMachines();   // 仅仅为了调试
                }
            }
        }
        void RecoveryMachine()
        {
            loadbalance_.OnlineMachine();
        }
    };
} // namespace ns_control

// 我自己写的废话：这里是和server.cc直接交互的模块，提供一些方法。
