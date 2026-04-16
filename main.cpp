#include "net/epollServer.hpp"

#include <cstdlib>
#include <iostream>

int main(int argc, char* argv[]) {
    int port = 6379;
    if (argc >= 2) {
        char* end = nullptr;
        const long parsed = std::strtol(argv[1], &end, 10);
        if (end == argv[1] || *end != '\0' || parsed <= 0 || parsed > 65535) {
            std::cerr << "invalid port: " << argv[1] << "\n";
            return 1;
        }
        port = static_cast<int>(parsed);
    }

    EpollServer server(port);
    if (!server.init()) {
        std::cerr << "server init failed\n";
        return 1;
    }

    server.run();
    return 0;
}
