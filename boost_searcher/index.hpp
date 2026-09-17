#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <mutex>
#include <unordered_map>
#include"util.hpp"
#include "log.hpp"
namespace ns_index{
    struct DocInfo{
        std::string title;   //标题
        std::string content; //去标签内容
        std::string url;     //官网文档url
        uint64_t doc_id;          //文档的ID 
    };
    struct InvertedElem{
        uint64_t doc_id;
        std::string word;
        int weight;
    };
    
    //倒排拉链
    typedef std::vector<InvertedElem> InvertedList;
    
    class Index{
    private:
    //正排索引    
        std::vector<DocInfo> forward_index;
    //倒排索引一定是一个关键字和一组（个）InvertedElem对应
        std::unordered_map<std::string,InvertedList> inverted_index;
    private:
        Index(){};
        Index(const Index&) = delete;
        Index& operator=(const Index&) =delete;
        
        static std::mutex mtx;
        static Index* instance;
    public:
        ~Index(){}
        static Index* GetInstance(){
            
            if(nullptr == instance){
                mtx.lock();
            if(nullptr == instance){
                
                instance = new Index();
            }
            mtx.unlock();
        }
            return instance;
        }
        //根据doc_id找到文档内容
        DocInfo* GetForwardIndex(uint64_t doc_id){
            if(doc_id >= forward_index.size()){
                std::cerr<<"doc_id out of range,error!"<<std::endl;
                return nullptr;
            }
            return &forward_index[doc_id];
        }
        //根据关键字string获得倒排拉链
        InvertedList* GetInvertedList(const std::string &word){
            auto iter = inverted_index.find(word);
            if(iter == inverted_index.end()){
                std::cerr<<word<<" have no InvertedList"<<std::endl;
                return nullptr;
            }
            return &(iter->second);
        }
        //根据去标签格式化之后的文档构建正排和倒排索引
        bool BuildIndex(const std::string &input){
            std::ifstream in(input, std::ios::in | std::ios::binary);
            if(!in.is_open()){
                std::cerr<<"sorry, "<<input<<" open error!"<<std::endl;
                return false;
            }

            std::string line;
            int count = 0;
            while(std::getline(in,line)){
                DocInfo* doc = BuildForwardIndex(line);
                if(doc == nullptr){
                    std::cerr<<"build "<<line<<" error!" <<std::endl;
                    continue;
                }
                BuildInvertedIndex(*doc);
                 ++count;
                if(count %100 == 0){
                    //std::cout<<"当前已经建立的索引文档"<<count<<std::endl;
                    LOG(NORMAL, "当前已经建立的索引文档"+std::to_string(count));
                }
            }
            
            return true;
        }
        private:
            DocInfo* BuildForwardIndex(const std::string &line){
                //解析line，字符串切分
                //line-> 3个string:title content url
                const std::string sep = "\3";
                std::vector<std::string> results;
                ns_util::StringUtil::Split(line , &results,sep);
                if(results.size() != 3){
                    return nullptr;
                }

                //字符串进行填充到DocInfo
                DocInfo doc;
                doc.title = results[0]; //title
                doc.content = results[1];//content
                doc.url = results[2];   //url
                doc.doc_id = forward_index.size(); //先保存id再插入，对应id就是当前doc在vector中的下标
                //插入到正排索引的vector
                forward_index.push_back(std::move(doc));
                return &forward_index.back();
            }
            bool BuildInvertedIndex(const DocInfo& doc){
                //word->倒排拉链
                struct word_cnt{
                    int title_cnt;
                    int content_cnt;

                    word_cnt():title_cnt(0),content_cnt(0){}
                };
                std::unordered_map<std::string, word_cnt> word_map;//暂存词频的映射表
                
                //title的分词
                std::vector<std::string> title_words;
                ns_util::JiebaUtil::CutString(doc.title,&title_words);
                //title的词频统计
                for(auto &s: title_words){
                    boost::to_lower(s);  //分词统一转换成小写
                    word_map[s].title_cnt++;
                }

                //对content的分词
                std::vector<std::string> content_words;
                ns_util::JiebaUtil::CutString(doc.content,&content_words);
                //content的词频统计
                for(auto &s : content_words){
                    boost::to_lower(s);  ////分词统一转换成小写
                    word_map[s].content_cnt++;
                }
            
#define X 10
#define Y 1 
                for(auto &word_pair :word_map){
                    InvertedElem item;
                    item.doc_id = doc.doc_id;
                    item.word = word_pair.first;
                    item.weight = X*word_pair.second.title_cnt + Y*word_pair.second.content_cnt;
                     //创建倒排拉链
                    InvertedList &inverted_list = inverted_index[word_pair.first];
                    inverted_list.push_back(std::move(item));  
                }
                return true;
            }
    };
    std::mutex Index::mtx;
    Index* Index::instance =nullptr;
} 

