#pragma once
#include<algorithm>
#include<unordered_map>
#include"index.hpp"
#include "util.hpp"
#include <jsoncpp/json/json.h>
#include "log.hpp"
namespace ns_searcher{
    struct InvertedElemPrint{
        uint64_t doc_id;
        int weight;
        std::vector<std::string> words;
        InvertedElemPrint():doc_id(0),weight(0){}
    };
    class Searcher{
        private:
            ns_index::Index *index;
        public:
            Searcher(){}
            ~Searcher(){}
        
            void InitSearcher(const std::string& input){
                //1.获取或者创建index对象
                index = ns_index::Index::GetInstance();
                // std::cout<<"获取index单例成功"<<std::endl;
                LOG(NORMAL,"获取index单例成");
                //2.根据index对象建立索引
                index->BuildIndex(input);
                // std::cout<<"建立正排和倒排索引成功"<<std::endl;
                LOG(NORMAL,"建立正排和倒排索引成功");

            }   
            //quer::搜索关键字
            //json_string：返回给用户浏览器的搜索结果 
            void Search(const std::string& query,std::string* json_string){
                
                //1.分词：对query进行按照searcher的要求进行分词
                std::vector<std::string> words;
                ns_util::JiebaUtil::CutString(query,&words);
                //2.触发：根据分词的各个词，进行index查找
                //ns_index::InvertedList inverted_list_all;
                std::vector<InvertedElemPrint> inverted_list_all;

                std::unordered_map<uint64_t,InvertedElemPrint> tokens_map;

                for(std::string word :words){
                    boost::to_lower(word);
                    ns_index::InvertedList *inverted_list = index->GetInvertedList(word);
                    if(nullptr == inverted_list){
                        continue;
                    }
                    //会有重复搜搜结果
                    //inverted_list_all.insert(inverted_list_all.end(),inverted_list->begin(),inverted_list->end());
                    //解决方案 
                    for(const auto& elem : *inverted_list){
                        auto& item = tokens_map[elem.doc_id]; //存在直接返回这个对象，不存在则新建
                        //item一定是doc_id相同的print节点
                        item.doc_id =elem.doc_id;
                        item.weight +=elem.weight;
                        item.words.push_back(elem.word);
                    }
                }
                for(const auto &item : tokens_map){
                    inverted_list_all.push_back(std::move(item.second));
                }
                //3.合并排序：汇总查找结果，按照相关性（weight）进行降序排序
                // std::sort(inverted_list_all.begin(),inverted_list_all.end(),\
                // [](const ns_index::InvertedElem& e1,\
                //    const ns_index::InvertedElem& e2){
                //     return e1.weight>e2.weight;
                // });
                std::sort(inverted_list_all.begin(),inverted_list_all.end(),[](const InvertedElemPrint& e1,const InvertedElemPrint& e2){
                    return e1.weight > e2.weight;
                });
    
                //4.构建：根据查找结果构建json串——jsoncpp
                Json::Value root;
                for(auto& item: inverted_list_all){
                     ns_index::DocInfo* doc = index->GetForwardIndex(item.doc_id);
                     if(doc == nullptr){
                        continue;
                     }
                     //使用json库来进行序列化
                    Json::Value elem;
                    elem["title"] = doc->title;
                    elem["desc"] = GetDesc(doc->content,item.words[0]); 
                    elem["url"] = doc->url;
                    
                    root.append(elem);
                }
                
                //Json::StyledWriter writer;
                Json::FastWriter writer;
                *json_string = writer.write(root);
            }
            std::string GetDesc(const std::string &html_content , const std::string word){
                //找到word在html_content中的首次出现
                //往前找50字节（没有五十个就从begin开始），往后找100字节
                const int prev_step = 50;
                const int next_step = 100;
                //1.找到首次出现
                auto iter = std::search(html_content.begin(),html_content.end(),word.begin(),word.end(),[](int x,int y){
                    return (std::tolower(x) == std::tolower(y));
                });
                if(iter == html_content.end()){
                    return "None1";
                }
                std::size_t pos = std::distance(html_content.begin(),iter);

                // std::size_t pos = html_content.find(word);
                // if(pos == std::string::npos){
                //     return "None1";  //没找到(理论上不可能出现)
                // }

                //2.获取start，end
                int start = 0;
                int end = html_content.size();
                //如果之前有50+字符，就更新开始位置
                if(pos > start + prev_step){
                    start = pos - prev_step;
                }
                if(pos < end - next_step){
                    end = pos + next_step;
                }
                //3.截取字串，return
                if(start >= end){
                    return "None2";
                }
                std::string desc = html_content.substr(start,end-start);
                desc += "...";
                return desc;
            }
    };
}
