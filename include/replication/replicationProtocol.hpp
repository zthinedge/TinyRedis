#pragma once

#include <exception>
#include <string>
//主从复制的通信协议工具包
namespace ReplicationProtocol {
//生成快照结束标记（主节点）
inline std::string snapshotEndMarker(long long offset) {
    return "TINYREDIS-SNAPSHOT-END " + std::to_string(offset);
}
//把字符串转换为64位整数
inline bool parseInt64(const std::string& s, long long& out) {
    try {
        size_t parsed = 0;
        const long long value = std::stoll(s, &parsed, 10);
        if (parsed != s.size()) {
            return false;
        }
        out = value;
        return true;
    } catch (const std::exception&) {
        return false;
    }
}
//解析快照结束标记（从节点）
inline bool parseSnapshotEndMarker(const std::string& s, long long& offset) {
    const std::string prefix = "TINYREDIS-SNAPSHOT-END ";
    if (s.rfind(prefix, 0) != 0) {
        return false;
    }
    return parseInt64(s.substr(prefix.size()), offset);
}
//解析全量同步（从节点）
inline bool parseFullResync(const std::string& s, std::string& replId, long long& offset) {
    //比如 FULLRESYNC abc123 200 
    const std::string prefix = "FULLRESYNC ";
    if (s.rfind(prefix, 0) != 0) {
        return false;
    }

    const size_t replIdStart = prefix.size();
    const size_t space = s.find(' ', replIdStart);
    if (space == std::string::npos || space == replIdStart) {
        return false;
    }

    replId = s.substr(replIdStart, space - replIdStart);
    return parseInt64(s.substr(space + 1), offset);
}
//解析增量同步（从节点）
inline bool parseContinue(const std::string& s, std::string& replId) {
    const std::string prefix = "CONTINUE";
    if (s == prefix) {
        replId.clear();
        return true;
    }
    const std::string prefixWithSpace = "CONTINUE ";
    if (s.rfind(prefixWithSpace, 0) != 0) {
        return false;
    }
    replId = s.substr(prefixWithSpace.size());
    return !replId.empty();
}

} // namespace ReplicationProtocol
