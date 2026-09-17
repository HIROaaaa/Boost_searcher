# boost_searcher — Boost 文档搜索引擎

基于 **C++11** 实现的离线文档搜索引擎，对本地下载的 [Boost 1.91.0](https://www.boost.org/doc/libs/1_91_0/) 官方 HTML 文档建立索引，提供：

- 🔍 **全文搜索**：jieba 分词 + 正排/倒排索引 + 相关性权重排序
- 🌐 **HTTP 搜索服务**：基于 httplib，前端网页搜索（端口 8081）
- 🤖 **AI 函数速查**：调用 DeepSeek API 解释 Boost 函数（参数 / 返回值 / 作用）

## 目录结构

```
boost_searcher/
├── cppjieba -> /home/ubuntu/thirdpart/cppjieba-master/include/cppjieba   # 软链接①
├── dict     -> /home/ubuntu/thirdpart/cppjieba-master/dict/              # 软链接②
├── parser.cc        # 网页解析器：扫描 data/input 下所有 HTML，提取标题/正文/URL
├── index.hpp        # 索引构建：正排索引 + 倒排索引（单例），词频权重 title×10 + content×1
├── searcher.hpp     # 搜索核心：分词 → 倒排查重合并 → 按权重降序 → jsoncpp 序列化
├── util.hpp         # 工具类：文件读取、字符串切分、jieba 分词封装
├── log.hpp          # 简易日志宏
├── ai_helper.hpp    # DeepSeek API 封装（https，SSLClient）
├── http_server.cc   # HTTP 服务入口：/s 搜索接口 + /ai_summary AI 接口
├── debug.cc         # 命令行测试客户端（循环输入查询词）
├── httplib.h        # 第三方 HTTP 库（已内置，无需安装）
├── deepseek.key     # DeepSeek API Key（可选，也可用环境变量）
├── makefile
├── data/
│   ├── input/           # 原始 Boost 文档 HTML（173 个文件）
│   └── raw_html/raw.txt # parser 生成的中间文件（\3 分隔：title\3content\3url）
├── wwwroot/index.html   # 前端搜索页面
└── test/                # 测试目录（含 cppjieba 源码包）
```

## ⚠️ 两个软链接（关键！）

本项目**不包含** cppjieba 源码，而是通过两个软链接引用位于 `/home/ubuntu/thirdpart/cppjieba-master` 的第三方库：

| 软链接 | 指向 | 作用 |
|---|---|---|
| `cppjieba` | `/home/ubuntu/thirdpart/cppjieba-master/include/cppjieba` | 提供 jieba 分词头文件，使 `#include "cppjieba/Jieba.hpp"` 生效 |
| `dict` | `/home/ubuntu/thirdpart/cppjieba-master/dict/` | 提供分词所需词典（jieba.dict.utf8、hmm_model.utf8、user.dict.utf8、idf.utf8、stop_words.utf8），运行时按 `./dict/...` 相对路径加载 |

> 软链接指向的是**绝对路径**，换机器或移动目录后需要重建：

```bash
# 假设 cppjieba 源码在 /path/to/cppjieba-master
ln -s /path/to/cppjieba-master/include/cppjieba cppjieba
ln -s /path/to/cppjieba-master/dict dict
```

缺失这两个链接会导致：编译时找不到 `cppjieba/Jieba.hpp`，运行时找不到词典文件而崩溃。

## 环境依赖

- g++（支持 C++11）
- Boost 库：`libboost-system-dev libboost-filesystem-dev`（另用到 boost/algorithm/string.hpp，随头文件一起安装）
- jsoncpp：`libjsoncpp-dev`
- OpenSSL：`libssl-dev`（仅 AI 功能需要，用于 HTTPS 调用 DeepSeek）
- cppjieba：见上方软链接说明

## 编译

```bash
make          # 生成三个可执行文件：parser、debug、http_server
make clean    # 清理可执行文件
```

## 使用流程

### 1. 解析网页（生成索引数据）

```bash
./parser
```

扫描 `data/input` 下的 HTML 文档，解析出标题、去标签正文、URL，输出到 `data/raw_html/raw.txt`（仓库中已附带生成好的 21MB 数据，可跳过此步）。

### 2. 命令行测试搜索

```bash
./debug
```

循环等待输入查询词，输出 JSON 格式搜索结果（doc_id 合并去重、按权重降序）。

### 3. 启动 HTTP 服务

```bash
./http_server
```

启动后在浏览器访问 `http://localhost:8081`：

- **搜索**：`GET /s?word=关键字` → 返回 JSON 数组，每项含 `title` / `desc`（命中词前后截取的摘要）/ `url`
- **AI 函数解释**：`GET /ai_summary?func=函数名` → 返回 `{"summary": "..."}`，由 DeepSeek 生成中文解释

### 4. 配置 DeepSeek API Key（AI 功能）

二选一：

```bash
export DEEPSEEK_API_KEY=sk-xxx     # 环境变量（优先）
# 或
echo -n "sk-xxx" > deepseek.key    # 本地文件（deepseek.key，注意勿提交到 git）
```

未配置 Key 时，`/ai_summary` 会返回提示信息，不影响搜索功能。

## 实现要点

- **索引结构**：正排索引（`vector<DocInfo>`，doc_id 即下标）+ 倒排索引（`unordered_map<word, InvertedList>`，单例懒加载 + 双检锁）
- **分词**：cppjieba 的 `CutForSearch`（搜索引擎模式），统一转小写
- **权重**：`weight = 10 × 标题词频 + 1 × 正文词频`
- **搜索**：对查询分词后合并各词的倒排拉链，按 doc_id 聚合累加权重，降序输出前若干条
- **AI 总结**：httplib SSLClient 请求 `api.deepseek.com/chat/completions`（模型 deepseek-chat，temperature 0.3，要求 100 字内简洁回答）
