#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <utility>

//节点的角色类型 主节点 / 从节点
enum class ReplicationRole {
    Master,     //主节点（写）
    Replica,        //从节点（只读，同步主节点数据）
};

struct ReplicationState {
    //主节点id 供从节点进行复制，同时进行身份校验
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
    bool masterLinkUp = false;      //是否连上主节点
    bool hasCachedMasterState = false;
    int connectedReplicas = 0;
    std::string masterReplId = generateReplId();
    long long masterReplOffset = 0;         //主节点全局总复制偏移量
    size_t replBacklogSize = 1024 * 1024;
    std::string replBacklog;        //复制积压缓冲区 主节点专属
    long long replBacklogFirstByteOffset = 1;       //缓冲区第一个字节的全局偏移量

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
    //主节点把需要同步的数据追加到复制积压缓冲区
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
    //判断是否可以进行增量同步，True为可以 ，False则进行全量同步
    bool canPartialResync(const std::string& replId, long long offset) const {
        //offset为已经从节点同步完的最后一个位置
        if (replId != masterReplId || offset < 0 || offset > masterReplOffset) {
            return false;
        }
        if (replBacklog.empty()) {
            return offset == masterReplOffset;
        }
        return offset >= replBacklogFirstByteOffset - 1;
    }
    //在主节点缓冲区中截取的数据
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
