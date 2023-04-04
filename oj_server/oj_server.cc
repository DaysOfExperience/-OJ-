#include <iostream>
#include <string>
#include <signal.h>
#include "../common/httplib.h"
#include "./oj_control.hpp"

using namespace httplib;
using namespace ns_control;

Control *ctrl_ptr = nullptr;

void Recovery(int signo)
{
    ctrl_ptr->RecoveryMachine();
}

int main()
{
    // 用户请求的服务路由功能
    Server svr;
    Control ctrl;
    ctrl_ptr = &ctrl;
    // std::cout << "hhhhh1" << std::endl;
    signal(SIGQUIT, Recovery);

    // 功能1: 获取所有的题目列表(严格来说，这里需要返回的是所有题目列表构成的一个html网页)
    // 指的是访问这个all_questions资源时，会进行如下响应
    svr.Get("/all_questions", [&ctrl](const Request &req, Response &resp){
        // 返回一张包含所有题目的html网页
        std::string html;
        ctrl.AllQuestions(&html);
        resp.set_content(html, "text/html; charset=utf-8");
    });
    // std::cout << "hhhhh2" << std::endl;

    // 功能2: 用户根据题目编号，获取指定的题目内容（某指定题目的内容所组成的一个html网页）
    // /question/100 -> 正则匹配（不懂）  ?????????
    // R"()", 原始字符串raw string, 保持字符串内容的原貌, 不用做相关的转义
    svr.Get(R"(/question/(\d+))", [&ctrl](const Request &req, Response &resp){
        std::string number = req.matches[1];  // ???????
        std::string html;
        ctrl.OneQuestion(number, &html);
        resp.set_content(html, "text/html; charset=utf-8");
    });
    // std::cout << "hhhhh3" << std::endl;

    // 用户提交代码，使用我们的判题功能
    svr.Post(R"(/judge/(\d+))", [&ctrl](const Request &req, Response &resp){
        std::string number = req.matches[1];  // ???????(题目编号)
        std::string result_json;  // 编译运行结果，也就是OJ程序的测试结果，非html
        ctrl.Judge(number, req.body, &result_json);
        resp.set_content(result_json, "application/json;charset=utf-8");
        // std::cout << "out_json : " << result_json << std::endl;
        // resp.set_content("指定题目的判题功能: " + number, "text/plain; charset=utf-8");
    });
    // std::cout << "hhhhh4" << std::endl;
    svr.set_base_dir("./wwwroot");  // 首页设置，也就是访问127.0.0.1:8080时，要获取的资源
    svr.listen("0.0.0.0", 8080);  // 端口号直接固定，因为不像编译服务一样要部署到多台机器上
    return 0;
}