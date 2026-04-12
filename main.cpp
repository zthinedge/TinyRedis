#include "net/epollServer.hpp"

#include <iostream>

int main() {
    EpollServer server(6379);
    if (!server.init()) {
        std::cerr << "server init failed\n";
        return 1;
    }

    server.run();
    return 0;
}
