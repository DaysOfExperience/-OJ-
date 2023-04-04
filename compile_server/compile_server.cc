#include "compile_run.hpp"
#include "../common/httplib.h"

using namespace ns_compile_run;
using namespace httplib;

static void Usage(const std::string &proc)
{
    std::cout << "\nUsage: " << proc << " port\n" << std::endl;
}

// ./compile_server post
int main(int argc, char *argv[])
{
    if(argc != 2)
    {
        Usage(argv[0]);
    }
    Server svr;
    // 这里...接收一个req，返回一个resp？不太懂说实话，不过暂时来看不重要。
    // 感觉...就是cpp-httplib的接口使用方法可能
    // 网页，客户端访问/hello时就会有对应数据返回。
    // svr.Get("/hello", [](const Request &req, Response &resp){
    //     // 设置响应正文？或许
    //     resp.set_content("hello httplib, 你好 httplib!", "text/plain;charset=utf-8");
    // });

    // 说实话，有关这里post，get的相关http内容，有印象，但是记不太清了，好像是请求和响应的方法？
    // 我记得是，客户端也就是浏览器请求时可以是post/get方法，区别是账号密码的传输方式。一种是在正文中，一种是在url中。
    // 而http服务端响应时，一般都是post方法？
    // 那下面的req和resp这两个参数如何理解呢？
    svr.Post("/compile_and_run", [](const Request &req, Response &resp)
    {
        // 用户请求的服务正文正是我们想要的json string
        std::string in_json = req.body;
        std::string out_json;
        if(!in_json.empty())
        {
            CompileAndRun::execute(in_json, &out_json);
            resp.set_content(out_json, "application/json;charset=utf-8");
        }
    });
    
    // 不需要首页信息，不是一个对外网站。上面只是用来测试，介绍如何使用httplib库
    // svr.set_base_dir("./wwwroot");

    svr.listen("0.0.0.0", atoi(argv[1]));
    
    return 0;
}

// // 进度:cpu时间限制失效  已解决，不能sleep，sleep算进程处于休眠状态。
// // 删除临时文件的工作还没做
// // 测试还没完,最初的成功了
// int main()
// {
//     Json::Value value;
//     value["code"] = R"(
//     #include <iostream>
//     #include <unistd.h>
//     int main()
//     {
//         std::cout << "哈哈哈" << std::endl;
//         std::cerr << "呵呵呵" << "\n";
//         return 0;
//     }
//     )";
//     value["input"] = "";
//     value["cpu_limit"] = 1;
//     value["mem_limit"] = 10240 * 3;
//     Json::FastWriter writer;
//     std::string in_json = writer.write(value);
//     std::string out_json;
//     CompileAndRun::execute(in_json, &out_json);
//     std::cout << out_json << std::endl;
//     return 0;
// }


/*
// // #level表示把level转换成字符串常量。例如，如果level是DEBUG，则#level将被替换为"DEBUG"。@1
// #include <iostream>
// #define TEST(haha) test(#haha)
// void test(std::string s)
// {
//     std::cout << s << std::endl;
// }

// int main()
// {
//     TEST(suiyi);
//     return 0;
// }

#include "../common/log.hpp"
using namespace ns_log;
enum{
    haha,
    hehe
};
int main()
{
    LOG(INFO) << std::endl;
    // std::cout << haha << std::endl;
    return 0;
}


// 测试Log 宏定义... 等问题
*/