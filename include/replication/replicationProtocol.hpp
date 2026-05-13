#pragma once

#include <exception>
#include <string>

namespace ReplicationProtocol {

inline std::string snapshotEndMarker(long long offset) {
    return "TINYREDIS-SNAPSHOT-END " + std::to_string(offset);
}

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

inline bool parseSnapshotEndMarker(const std::string& s, long long& offset) {
    const std::string prefix = "TINYREDIS-SNAPSHOT-END ";
    if (s.rfind(prefix, 0) != 0) {
        return false;
    }
    return parseInt64(s.substr(prefix.size()), offset);
}

inline bool parseFullResync(const std::string& s, std::string& replId, long long& offset) {
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
