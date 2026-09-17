#include "searcher.hpp"
#include "httplib.h"
#include "ai_helper.hpp"

const std::string root_path = "./wwwroot";
const std::string input = "data/raw_html/raw.txt";
int main(){

    ns_searcher::Searcher search;
    search.InitSearcher(input);
    

    httplib::Server svr;
    svr.set_base_dir(root_path.c_str());
    svr.Get("/s", [&search](const httplib::Request& req, httplib::Response& rsp){
            if(!req.has_param("word")){
                rsp.set_content("必须要有搜索关键字","text/plain; charset=utf-8");
                return;
            }
            std::string word = req.get_param_value("word");
            std::cout<<"用户正在搜索："<<word<<std::endl;
            LOG(NORMAL,"用户正在搜索："+word);
            std::string json_string; 
            search.Search(word,&json_string);
            rsp.set_content(json_string,"application/json");
            //rsp.set_content("hello world","text/plain; charset=utf-8");
            });
    // ===== AI 函数总结接口 =====
    svr.Get("/ai_summary", [](const httplib::Request& req, httplib::Response& rsp){
            if(!req.has_param("func")){
                Json::Value err;
                err["summary"] = "缺少函数名";
                Json::FastWriter w;
                rsp.set_content(w.write(err), "application/json; charset=utf-8");
                return;
            }
            std::string func = req.get_param_value("func");
            LOG(NORMAL, "AI 查询函数: " + func);
            std::string summary = GetAISummary(func);

            Json::Value root;
            root["summary"] = summary;
            Json::FastWriter writer;
            rsp.set_content(writer.write(root), "application/json; charset=utf-8");
            });
            LOG(NORMAL,"服务器启动成功");
    svr.listen("0.0.0.0",8081);
    return 0;
}
