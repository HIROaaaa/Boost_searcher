#pragma once
#include <string>
#include <cstdlib>
#include "httplib.h"
#include <jsoncpp/json/json.h>
#include "log.hpp"

// 从环境变量或文件读取 DeepSeek API Key
static std::string GetApiKey() {
    const char* env = getenv("DEEPSEEK_API_KEY");
    if (env && env[0]) return env;

    // 尝试从配置文件读取
    FILE* f = fopen("./deepseek.key", "r");
    if (f) {
        char buf[256] = {0};
        if (fgets(buf, sizeof(buf), f)) {
            size_t len = strlen(buf);
            while (len > 0 && (buf[len-1] == '\n' || buf[len-1] == '\r')) buf[--len] = 0;
            fclose(f);
            return buf;
        }
        fclose(f);
    }
    return "";
}

// 调用 DeepSeek API 获取函数总结
static std::string GetAISummary(const std::string& func_name) {
    std::string api_key = GetApiKey();
    if (api_key.empty()) {
        return "⚠️ 未配置 DeepSeek API Key";
    }

    // 构建 JSON body
    Json::Value msg, messages(Json::arrayValue), root;
    msg["role"] = "user";
    msg["content"] = "你是一个C++ Boost库专家。请用中文解释以下Boost函数：参数类型、返回值、作用。格式：参数→xxx | 返回值→xxx | 作用→xxx"
                     + func_name + "。控制在100字以内，简洁明了。";
    messages.append(msg);
    root["model"] = "deepseek-chat";
    root["messages"] = messages;
    root["max_tokens"] = 150;
    root["temperature"] = 0.3;

    Json::FastWriter writer;
    std::string body = writer.write(root);

    // 使用 httplib 的 SSLClient 调用 DeepSeek API
    httplib::SSLClient client("api.deepseek.com", 443);
    client.set_connection_timeout(0, 500000);    // 500ms 连接超时
    client.set_read_timeout(0, 5000000);         // 5s 读取超时
    client.enable_server_certificate_verification(false);

    httplib::Headers headers = {
        {"Authorization", "Bearer " + api_key},
        {"Content-Type", "application/json"}
    };

    auto res = client.Post("/chat/completions", headers, body, "application/json");
    if (!res) {
        LOG(WARNING, "AI 请求失败");
        return "⚠️ AI 请求失败";
    }
    if (res->status != 200) {
        LOG(WARNING, "AI 返回状态码: " + std::to_string(res->status));
        return "⚠️ AI 服务暂时不可用";
    }

    // 用 jsoncpp 解析返回 JSON
    Json::Value resp_root;
    Json::Reader reader;
    if (!reader.parse(res->body, resp_root)) {
        return "⚠️ 解析 AI 响应失败";
    }

    try {
        std::string summary = resp_root["choices"][0]["message"]["content"].asString();
        return summary;
    } catch (...) {
        return "⚠️ 解析 AI 响应失败";
    }
}
