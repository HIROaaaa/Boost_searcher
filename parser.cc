#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <boost/filesystem.hpp>
#include "util.hpp"

//放着所有html网页的目录
const std::string  src_path="data/input";
const std::string output="data/raw_html/raw.txt";

typedef struct DocInfo{
    std::string title; //文档标题
    std::string content; //文档内容
    std::string url;   //官网urlp
}DocInfo_t;

bool EnumFile(const std::string &src_path,std::vector<std::string>* list);
bool ParseHtml(std::vector<std::string>& files_list,std::vector<DocInfo_t>* results);
bool SaveHtml(std::vector<DocInfo_t>& results,const std::string& output);

int main()
{
    std::vector<std::string> files_list;
    //将每个html文件名带着路径一起保存到file_list中
    if(!EnumFile(src_path,&files_list)){
        std::cerr<<"enum file name error!"<<std::endl;
        return 1;
    }

    //按照file_list读取每个文件的内容，并进行解析
    std::vector<DocInfo_t> results;
    if(!ParseHtml(files_list,&results)){
        std::cerr<<"parse html error!"<<std::endl;
        return 2;
    }
    //把解析完毕的各个文件内容写入到output
    if(!SaveHtml(results,output)){
        std::cerr<<"save html error!"<<std::endl;
        return 3;
    }
    return 0;
}

bool EnumFile(const std::string &src_path,std::vector<std::string>* files_list){
    namespace fs = boost::filesystem;
    fs::path root_path(src_path);

    //判断路径是否存在
    if(!fs::exists(root_path)){
        std::cerr<<src_path<<"no exists"<<std::endl;
        return false;
    }

    fs::recursive_directory_iterator end;
    for(fs::recursive_directory_iterator iter(root_path); iter!=end;iter++){
        //判断是否是普通文件
        if(!fs::is_regular_file(*iter)){
            continue;
        }
        //判断后缀
        if(iter->path().extension()!=".html"){
            continue;
        }

       // std::cout<<"debug:"<<iter->path().string()<<std::endl;
        //当前路径是合法的，以html结束的普通网页文件
        files_list->push_back(iter->path().string());  //将所有带路径的html保存到files_list
    }
    return true;
}



static bool ParseTitle(const std::string &file,std::string* title){
    std::size_t begin=file.find("<title>");
    if(begin == std::string::npos){
        return false;
    }
    std::size_t end=file.find("</title>");
    if(end == std::string::npos){
        return false;
    }
    begin += std::string("<title>").size();
    *title = file.substr(begin,end-begin);
    
    if(begin > end){
        return false;
    }
    return true;
}
static bool ParseContent(const std::string &file,std::string* content){
    //去标签，基于一个简易的状态机
    enum status{
        LABLE,
        CONTENT
    };
    
    enum status s=LABLE;
    for( char c:file){
        switch(s){
            case LABLE:
                if(c == '>') s = CONTENT;
                break;
            case CONTENT:
                if(c == '<') s=LABLE;
                else{
                    //不想要原始文件里的\n,要用\n作为html解析之后的分隔符
                    if(c == '\n') c=' ';
                    content->push_back(c);
                }
                break;
            default:
                break;
        }
    }
    return true;
}
static bool ParseUrl(const std::string &file_path ,std::string *url){
    std::string url_head = "https://www.boost.org/doc/libs/1_91_0/doc/html";
    std::string url_tail = file_path.substr(src_path.size());
    
    *url = url_head + url_tail;
    return true;
}
    

//for debug
// void ShowDoc(const DocInfo_t &doc)
// {
//     std::cout<<"title: "<< doc.title << std::endl;
//     std::cout << "content: "<<doc.content << std::endl;
//     std::cout << "url: "<<doc.url << std::endl;
// }


bool ParseHtml(std::vector<std::string>& files_list,std::vector<DocInfo_t>* results){
    for(const std::string &file :files_list){
        //读取文件
        std::string result;
        if(!ns_util::FileUtil::ReadFile(file,&result)){
            continue;
        }
        DocInfo_t doc;
        //解析文件，提取title
        if(!ParseTitle(result,&doc.title)){
            continue;
        }
        //提取content
        if(!ParseContent(result,&doc.content)){
            continue;
        }
        //提取路径，构建url
        if(!ParseUrl(file,&doc.url)){
            continue;
        }

        results->push_back(std::move(doc));
        
        //for debug
        //ShowDoc(doc);
    }    

    
    return true;
}

bool SaveHtml(std::vector<DocInfo_t>& results,const std::string& output){
#define SEP '\3'
     //二进制方式进行写入
    std::ofstream out(output,std::ios::out | std::ios::binary);
    if(!out.is_open()){
        std::cerr<<"open "<<output<<" failed"<<std::endl;
        return false;
    }

    //文件写入
    for(auto &item : results){
        std::string out_string;
        out_string = item.title;
        out_string += SEP;
        out_string += item.content;
        out_string += SEP;
        out_string += item.url;
        out_string += '\n';
        
        out.write(out_string.c_str(),out_string.size());
    }
    out.close();
    return true;

}

