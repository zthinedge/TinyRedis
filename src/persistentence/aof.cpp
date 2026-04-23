#include "../../include/persistentence/aof.hpp"

#include "../../include/command/commandParser.hpp"
#include "../../include/protocol/respEncoder.hpp"
#include "../../include/protocol/respParser.hpp"

#include <cerrno>
#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>

namespace {
bool writeAll(int fd, const char* data, size_t len, std::string& err) {
    size_t written = 0;
    while (written < len) {
        const ssize_t n = ::write(fd, data + written, len - written);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            err = std::strerror(errno);
            return false;
        }
        if (n == 0) {
            err = "short write";
            return false;
        }
        written += static_cast<size_t>(n);
    }
    return true;
}
} // namespace

const char* aofFsyncPolicyName(AofFsyncPolicy policy) {
    switch (policy) {
    case AofFsyncPolicy::Always:
        return "always";
    case AofFsyncPolicy::EverySec:
        return "everysec";
    case AofFsyncPolicy::No:
        return "no";
    }
    return "unknown";
}

AOF::AOF(bool enabled, std::string path, AofFsyncPolicy fsyncPolicy)
    : enabled_(enabled),
      path_(std::move(path)),
      fsyncPolicy_(fsyncPolicy),
      dirty_(false),
      lastFsync_(std::chrono::steady_clock::now()) {}

bool AOF::enabled() const {
    return enabled_;
}

void AOF::setEnabled(bool enabled) {
    enabled_ = enabled;
}

const std::string& AOF::path() const {
    return path_;
}

AofFsyncPolicy AOF::fsyncPolicy() const {
    return fsyncPolicy_;
}

bool AOF::appendCommand(const std::vector<std::string>& argv, std::string& err) {
    err.clear();
    if (!enabled_) {
        return true;
    }

    const std::string payload = RESPEncoder::array(argv);
    const int fd = ::open(path_.c_str(), O_CREAT | O_WRONLY | O_APPEND, 0644);
    if (fd < 0) {
        err = std::strerror(errno);
        return false;
    }

    bool ok = writeAll(fd, payload.data(), payload.size(), err);
    if (ok && fsyncPolicy_ == AofFsyncPolicy::Always && ::fsync(fd) != 0) {
        err = std::strerror(errno);
        ok = false;
    }

    if (::close(fd) != 0 && ok) {
        err = std::strerror(errno);
        ok = false;
    }

    if (ok && fsyncPolicy_ == AofFsyncPolicy::EverySec) {
        dirty_ = true;
    }

    return ok;
}

bool AOF::rewriteCommands(const std::vector<std::vector<std::string>>& commands, std::string& err) {
    err.clear();
    if (!enabled_) {
        return true;
    }

    const std::string tmpPath = path_ + ".tmp";
    const int fd = ::open(tmpPath.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd < 0) {
        err = std::strerror(errno);
        return false;
    }

    bool ok = true;
    for (const std::vector<std::string>& argv : commands) {
        const std::string payload = RESPEncoder::array(argv);
        if (!writeAll(fd, payload.data(), payload.size(), err)) {
            ok = false;
            break;
        }
    }

    if (ok && ::fsync(fd) != 0) {
        err = std::strerror(errno);
        ok = false;
    }

    if (::close(fd) != 0 && ok) {
        err = std::strerror(errno);
        ok = false;
    }

    if (!ok) {
        (void)::unlink(tmpPath.c_str());
        return false;
    }

    if (::rename(tmpPath.c_str(), path_.c_str()) != 0) {
        err = std::strerror(errno);
        (void)::unlink(tmpPath.c_str());
        return false;
    }

    dirty_ = false;
    lastFsync_ = std::chrono::steady_clock::now();
    return true;
}

bool AOF::flushIfNeeded(std::string& err, bool force) {
    err.clear();
    if (!enabled_ || fsyncPolicy_ != AofFsyncPolicy::EverySec || !dirty_) {
        return true;
    }

    const auto now = std::chrono::steady_clock::now();
    const auto elapsedMs =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - lastFsync_).count();
    if (!force && elapsedMs < 1000) {
        return true;
    }

    if (!fsyncPath(err)) {
        return false;
    }

    dirty_ = false;
    lastFsync_ = now;
    return true;
}

bool AOF::fsyncPath(std::string& err) {
    const int fd = ::open(path_.c_str(), O_RDWR);
    if (fd < 0) {
        err = std::strerror(errno);
        return false;
    }

    bool ok = true;
    if (::fsync(fd) != 0) {
        err = std::strerror(errno);
        ok = false;
    }
    if (::close(fd) != 0 && ok) {
        err = std::strerror(errno);
        ok = false;
    }
    return ok;
}

bool AOF::replay(const std::function<bool(const std::vector<std::string>&, std::string&)>& apply,
                 std::string& err) const {
    err.clear();
    if (!enabled_) {
        return true;
    }

    std::ifstream in(path_, std::ios::binary);
    if (!in.good()) {
        // AOF 文件不存在时视为无状态恢复成功。
        return true;
    }

    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();
    if (content.empty()) {
        return true;
    }

    RESPParser parser;
    parser.feed(content.data(), content.size());

    for (;;) {
        RESPObject obj;
        bool ok = false;
        try {
            ok = parser.parse(obj);
        } catch (const std::exception& ex) {
            err = std::string("AOF parse failed: ") + ex.what();
            return false;
        }

        if (!ok) {
            break;
        }

        std::vector<std::string> argv;
        std::string parseErr;
        if (!CommandParser::toArgv(obj, argv, parseErr)) {
            err = "AOF command parse failed: " + parseErr;
            return false;
        }

        std::string applyErr;
        if (!apply(argv, applyErr)) {
            err = "AOF replay command failed: " + applyErr;
            return false;
        }
    }

    return true;
}
