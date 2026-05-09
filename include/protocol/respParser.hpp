#pragma once
#include "respObject.hpp"
#include <string>

class RESPParser
{
public:

    RESPParser();

    // 喂数据
    void feed(const char* data, size_t len);

    // 尝试解析一个完整RESP
    bool parse(RESPObject& out);

    // 返回缓冲区中尚未消费的字节数，便于上层区分“等待更多数据”和“已完整消费完”。
    size_t pendingBytes() const;

private:

    std::string buffer_;
    size_t pos_;

private:

    bool parseInternal(RESPObject&);

    bool parseSimpleString(RESPObject&);
    bool parseError(RESPObject&);
    bool parseInteger(RESPObject&);
    bool parseBulkString(RESPObject&);
    bool parseArray(RESPObject&);

    bool readLine(std::string& line);
};
