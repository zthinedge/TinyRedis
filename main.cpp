#include "config/serverConfig.hpp"
#include "net/epollServer.hpp"

#include <iostream>
#include <string>

namespace {
bool parsePortArg(const std::string& value, int& port) {
    try {
        size_t parsedChars = 0;
        const long parsed = std::stol(value, &parsedChars, 10);
        if (parsedChars != value.size() || parsed <= 0 || parsed > 65535) {
            return false;
        }
        port = static_cast<int>(parsed);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

void printUsage(const char* program) {
    std::cerr << "usage: " << program << " [port] [config-file]\n"
              << "       " << program << " --config <config-file> [--port <port>]\n";
}

bool parseArgs(int argc, char* argv[], ServerConfig& config) {
    int portOverride = -1;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return false;
        }

        if (arg == "--config" || arg == "-c") {
            if (i + 1 >= argc) {
                std::cerr << "--config requires a file path\n";
                printUsage(argv[0]);
                return false;
            }
            std::string err;
            if (!loadServerConfig(argv[++i], config, err)) {
                std::cerr << err << "\n";
                return false;
            }
            continue;
        }

        if (arg == "--port" || arg == "-p") {
            if (i + 1 >= argc || !parsePortArg(argv[++i], portOverride)) {
                std::cerr << "--port requires a valid port in 1..65535\n";
                printUsage(argv[0]);
                return false;
            }
            continue;
        }

        int parsedPort = 0;
        if (parsePortArg(arg, parsedPort)) {
            portOverride = parsedPort;
            continue;
        }

        std::string err;
        if (!loadServerConfig(arg, config, err)) {
            std::cerr << err << "\n";
            return false;
        }
    }

    if (portOverride > 0) {
        config.port = portOverride;
    }

    return true;
}
} // namespace

int main(int argc, char* argv[]) {
    ServerConfig config;
    if (!parseArgs(argc, argv, config)) {
        return 1;
    }

    EpollServer server(config);
    if (!server.init()) {
        std::cerr << "server init failed\n";
        return 1;
    }

    server.run();
    return 0;
}
