#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <utility>

enum class ReplicationRole {
    Master,
    Replica,
};

struct ReplicationState {
    static std::string generateReplId() {
        uint64_t x = static_cast<uint64_t>(
            std::chrono::high_resolution_clock::now().time_since_epoch().count());
        x ^= static_cast<uint64_t>(reinterpret_cast<uintptr_t>(&x));

        const char* hex = "0123456789abcdef";
        std::string out;
        out.reserve(40);
        for (int i = 0; i < 40; ++i) {
            x ^= x << 7;
            x ^= x >> 9;
            x ^= x << 8;
            out.push_back(hex[x & 0x0f]);
        }
        return out;
    }

    ReplicationRole role = ReplicationRole::Master;
    std::string masterHost;
    int masterPort = 0;
    bool masterLinkUp = false;
    bool hasCachedMasterState = false;
    int connectedReplicas = 0;
    std::string masterReplId = generateReplId();
    long long masterReplOffset = 0;
    size_t replBacklogSize = 1024 * 1024;
    std::string replBacklog;
    long long replBacklogFirstByteOffset = 1;

    void becomeMaster() {
        role = ReplicationRole::Master;
        masterHost.clear();
        masterPort = 0;
        masterLinkUp = false;
        hasCachedMasterState = false;
    }

    void becomeReplica(std::string host, int port) {
        role = ReplicationRole::Replica;
        masterHost = std::move(host);
        masterPort = port;
        masterLinkUp = false;
        hasCachedMasterState = false;
    }

    bool isReplica() const {
        return role == ReplicationRole::Replica;
    }

    void appendBacklog(const std::string& payload) {
        if (payload.empty()) {
            return;
        }

        if (replBacklog.empty()) {
            replBacklogFirstByteOffset = masterReplOffset + 1;
        }

        replBacklog += payload;
        masterReplOffset += static_cast<long long>(payload.size());

        if (replBacklog.size() > replBacklogSize) {
            const size_t extra = replBacklog.size() - replBacklogSize;
            replBacklog.erase(0, extra);
            replBacklogFirstByteOffset += static_cast<long long>(extra);
        }
    }

    bool canPartialResync(const std::string& replId, long long offset) const {
        if (replId != masterReplId || offset < 0 || offset > masterReplOffset) {
            return false;
        }
        if (replBacklog.empty()) {
            return offset == masterReplOffset;
        }
        return offset >= replBacklogFirstByteOffset - 1;
    }

    std::string backlogAfter(long long offset) const {
        if (replBacklog.empty() || offset >= masterReplOffset) {
            return "";
        }
        const long long index = offset - replBacklogFirstByteOffset + 1;
        if (index < 0 || index > static_cast<long long>(replBacklog.size())) {
            return "";
        }
        return replBacklog.substr(static_cast<size_t>(index));
    }
};
