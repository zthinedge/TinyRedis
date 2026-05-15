#pragma once

#include "../protocol/respParser.hpp"

#include <string>

// 从节点同步主节点时使用的状态机
enum class MasterSyncState {
    Disconnected,       
    WaitingPong,        
    WaitingReplconf,    
    WaitingFullResync,  
    LoadingSnapshot,    
    Streaming,          
};

// 从节点与主节点之间的复制连接信息
struct MasterReplicationLink {
    int fd = -1;                                             
    RESPParser parser;                                       
    std::string writeBuf;                                    
    MasterSyncState state = MasterSyncState::Disconnected;   
    long long pendingFullResyncOffset = 0;                   
};
/*
复制握手流程(replica 和 master 建立复制关系的应用层握手）：

1. replica 主动 connect master
   - 当前结构体中的 fd 就是这条连接的 socket fd。
   - 这个 fd 不是从节点自己的 listenFd_，而是专门用于和 master 通信的复制连接。

2. replica -> master: PING
   - 作用：确认 master 存活，并确认 RESP 协议收发正常。
   - 状态：WaitingPong
   - 期望回复：+PONG

3. replica -> master: REPLCONF listening-port <port>
   - 作用：向 master 上报 replica 的复制相关信息。
   - TinyRedis 当前主要接受该命令并返回 OK。
   - 状态：WaitingReplconf
   - 期望回复：+OK

4. replica -> master: PSYNC ? -1 或 PSYNC <replid> <offset>
   - PSYNC ? -1：第一次同步，没有历史复制状态，请求全量同步。
   - PSYNC <replid> <offset>：断线重连后，尝试从指定 offset 继续部分重同步。
   - 状态：WaitingFullResync

5. master -> replica:
   - FULLRESYNC <replid> <offset>：进行全量同步，后续发送快照命令流。
     replica 进入 LoadingSnapshot。
   - CONTINUE <replid>：可以部分重同步，master 会补发 backlog 中缺失的命令。
     replica 直接进入 Streaming。

6. LoadingSnapshot -> Streaming
   - TinyRedis 全量同步不发送 RDB，而是发送 SET/HSET/EXPIRE 等命令流。
   - replica 回放这些命令，直到收到 TINYREDIS-SNAPSHOT-END <offset>。
   - 快照结束后进入 Streaming，后续持续接收 master 的写命令传播。
*/
