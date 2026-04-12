#include "command/commandDispatcher.hpp"
#include "command/commandParser.hpp"
#include "protocol/respEncoder.hpp"
#include "protocol/respParser.hpp"

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace {
bool sendAll(int fd, const std::string& data) {
    size_t sent = 0;
    while (sent < data.size()) {
        const ssize_t n = ::send(fd, data.data() + sent, data.size() - sent, 0);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            return false;
        }
        if (n == 0) {
            return false;
        }
        sent += static_cast<size_t>(n);
    }
    return true;
}
} // namespace

int main() {
    constexpr int kPort = 6379;
    constexpr int kBacklog = 16;
    constexpr size_t kReadBufSize = 4096;

    const int listenFd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd < 0) {
        std::cerr << "socket failed: " << std::strerror(errno) << "\n";
        return 1;
    }

    int opt = 1;
    if (::setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "setsockopt failed: " << std::strerror(errno) << "\n";
        ::close(listenFd);
        return 1;
    }

    sockaddr_in addr {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(kPort);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if (::bind(listenFd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::cerr << "bind 127.0.0.1:" << kPort << " failed: " << std::strerror(errno) << "\n";
        ::close(listenFd);
        return 1;
    }

    if (::listen(listenFd, kBacklog) < 0) {
        std::cerr << "listen failed: " << std::strerror(errno) << "\n";
        ::close(listenFd);
        return 1;
    }

    std::cout << "TinyRedis listening on 127.0.0.1:" << kPort << "\n";

    CommandDispatcher dispatcher;

    for (;;) {
        const int connFd = ::accept(listenFd, nullptr, nullptr);
        if (connFd < 0) {
            if (errno == EINTR) {
                continue;
            }
            std::cerr << "accept failed: " << std::strerror(errno) << "\n";
            break;
        }

        RESPParser parser;
        char buf[kReadBufSize];

        for (;;) {
            const ssize_t n = ::recv(connFd, buf, sizeof(buf), 0);
            if (n == 0) {
                break;
            }
            if (n < 0) {
                if (errno == EINTR) {
                    continue;
                }
                std::cerr << "recv failed: " << std::strerror(errno) << "\n";
                break;
            }

            parser.feed(buf, static_cast<size_t>(n));

            for (;;) {
                RESPObject obj;
                bool ok = false;
                try {
                    ok = parser.parse(obj);
                } catch (const std::exception& ex) {
                    const std::string errReply =
                        RESPEncoder::error(std::string("ERR protocol error: ") + ex.what());
                    sendAll(connFd, errReply);
                    ok = false;
                }

                if (!ok) {
                    break;
                }

                std::vector<std::string> argv;
                std::string err;
                std::string reply;
                if (!CommandParser::toArgv(obj, argv, err)) {
                    reply = RESPEncoder::error("ERR " + err);
                } else {
                    reply = dispatcher.dispatch(argv);
                }

                if (!sendAll(connFd, reply)) {
                    break;
                }
            }
        }

        ::close(connFd);
    }

    ::close(listenFd);
    return 0;
}
