#pragma once

#include "../protocol/respParser.hpp"

#include <string>

enum class MasterSyncState {
    Disconnected,
    WaitingPong,
    WaitingReplconf,
    WaitingFullResync,
    LoadingSnapshot,
    Streaming,
};

struct MasterReplicationLink {
    int fd = -1;
    RESPParser parser;
    std::string writeBuf;
    MasterSyncState state = MasterSyncState::Disconnected;
    long long pendingFullResyncOffset = 0;
};
