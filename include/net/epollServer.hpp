#pragma once

#include "../command/commandDispatcher.hpp"
#include "../protocol/respParser.hpp"

#include <string>
#include <unordered_map>

class EpollServer {
public:
    explicit EpollServer(int port = 6379);
    ~EpollServer();

    EpollServer(const EpollServer&) = delete;
    EpollServer& operator=(const EpollServer&) = delete;

    bool init();
    void run();

private:
    struct ClientSession {
        RESPParser parser;
        std::string writeBuf;
        bool closeAfterWrite = false;
    };

private:
    bool initListenSocket();
    bool initEpoll();

    static bool setNonBlocking(int fd);
    bool updateClientEvents(int fd, bool wantWrite);
    void closeClient(int fd);

    void acceptClients();
    void handleClientRead(int fd);
    void handleClientWrite(int fd);

private:
    int port_;
    int listenFd_;
    int epollFd_;

    CommandDispatcher dispatcher_;
    std::unordered_map<int, ClientSession> clients_;
};
