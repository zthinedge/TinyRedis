#include "../../include/command/commandDispatcher.hpp"

#include "../../include/protocol/respEncoder.hpp"
#include <algorithm>
#include <cctype>

namespace {
std::string toUpperCopy(const std::string& s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });
    return out;
}

std::string wrongArity(const std::string& cmd) {
    return RESPEncoder::error("ERR wrong number of arguments for '" + cmd + "' command");
}
} // namespace

std::string CommandDispatcher::dispatch(const std::vector<std::string>& argv) {
    if (argv.empty()) {
        return RESPEncoder::error("ERR empty command");
    }

    const std::string cmd = toUpperCopy(argv[0]);

    if (cmd == "PING") {
        if (argv.size() == 1) {
            return RESPEncoder::simpleString("PONG");
        }
        if (argv.size() == 2) {
            return RESPEncoder::bulkString(argv[1]);
        }
        return wrongArity("ping");
    }

    if (cmd == "SET") {
        if (argv.size() != 3) {
            return wrongArity("set");
        }
        db_.set(argv[1], argv[2]);
        return RESPEncoder::simpleString("OK");
    }

    if (cmd == "GET") {
        if (argv.size() != 2) {
            return wrongArity("get");
        }
        std::string value;
        if (!db_.get(argv[1], value)) {
            return RESPEncoder::nullBulk();
        }
        return RESPEncoder::bulkString(value);
    }

    if (cmd == "DEL") {
        if (argv.size() < 2) {
            return wrongArity("del");
        }
        long long removed = 0;
        for (size_t i = 1; i < argv.size(); ++i) {
            removed += db_.del(argv[i]);
        }
        return RESPEncoder::integer(removed);
    }

    return RESPEncoder::error("ERR unknown command '" + argv[0] + "'");
}
