PARSER := parser
DUG := debug
HTTP_SERVER=http_server
CC := g++
# 通用编译标准
STD := -std=c++11
# boost链接参数
BOOST_LIB := -lboost_system -lboost_filesystem
# cppjieba头文件路径
CPPJIEBA_INC := -I./cppjieba/include

.PHONY: all clean
all: $(PARSER) $(DUG) $(HTTP_SERVER)

# 编译网页解析程序parser
$(PARSER):parser.cc
	$(CC) -o $@ $^ $(STD) $(BOOST_LIB)

# 编译搜索服务程序search_server
$(DUG):debug.cc
	$(CC) -o $@ $^ $(STD) $(CPPJIEBA_INC) $(BOOST_LIB) -ljsoncpp

# 编译搜索服务程序http_server（含AI功能，需要SSL支持HTTPS）
$(HTTP_SERVER):http_server.cc
	$(CC) -o $@ $^ $(STD) $(CPPJIEBA_INC) $(BOOST_LIB) -ljsoncpp -lpthread -DCPPHTTPLIB_OPENSSL_SUPPORT -lssl -lcrypto

# 清理所有可执行文件
clean:
	rm -f $(PARSER) $(DUG) $(HTTP_SERVER)
