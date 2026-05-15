# TinyRedis
## 项目简介
TinyRedis 是一个基于 C++17 实现的 Redis 兼容内存数据库内核项目，目标是在贴近真实工程的前提下逐步复现 Redis 的核心能力。  
当前版本已打通 `epoll` 单线程事件循环、RESP2 编解码、命令解析与分发链路，并支持 String 基础命令、TTL 命令子集、AOF 持久化与简化版主从复制。  
项目重点关注模块化设计与可测试性（`net/protocol/command/core/object/persistentence` 分层），当前阶段优先完善工程化收口、稳定性增强与后续核心特性的演进路线。


## 开发环境
- C++17（`g++`/`clang++`）
- CMake >= 3.10
- GTest（`find_package(GTest REQUIRED)`）
- Linux（当前网络层基于 `epoll`）

## 架构设计

TinyRedis 当前采用单线程 `epoll` 事件循环模型，主请求链路按 `net -> protocol -> command -> storage -> core` 分层。AOF 不在普通读请求主链路上，只在启动恢复和写命令持久化时参与。


- 请求链路：`Client -> EpollServer -> RESPParser -> CommandParser -> CommandDispatcher -> InMemoryDB`
- 响应链路：`CommandDispatcher -> RESPEncoder -> ClientSession::writeBuf -> handleClientWrite -> Client`
- 持久化旁路：AOF 负责写命令追加、启动恢复、同步/后台重写与可配置 fsync 策略
- cron 任务：当前不是独立线程，而是在 `EpollServer::run` 事件循环中周期触发 `CommandDispatcher::cron`

### 架构图

![TinyRedis 架构图](docs/assets/v0.1.png)

## 当前能力概览
- 已实现命令：`PING/SET/MSET/GET/MGET/DEL/EXISTS/INCR/INCRBY/DECR/EXPIRE/TTL/PTTL/PERSIST/INFO/REWRITEAOF/BGREWRITEAOF`
- 底层结构：实现 SDS 和 DICT，其中 DICT 支持双哈希表渐进式 rehash，读写删除操作会顺带推进迁移，降低单次扩容抖动
- 过期策略：惰性过期（访问时检查）+ 主动过期（事件循环周期触发抽样清理）
- 配置：支持配置文件和启动参数设置端口、AOF 开关、AOF 文件路径、`appendfsync` 策略和 `replicaof` 复制角色
- 观测：支持 `INFO` 输出 server、clients、stats、persistence、replication 基础指标
- 复制：支持简化版 master/replica，全量快照命令流同步、后续写命令传播、replication backlog、partial resync 和断线重连
- 持久化：AOF（写命令追加 + 启动重放恢复 + `REWRITEAOF/BGREWRITEAOF` + `always/everysec/no` fsync 策略）
- 测试基线：`test_sds`、`test_dict`、`test_resp`、`test_config`、`test_command`、`test_aof`、`test_e2e`（已接入 CTest）

## 项目亮点
- 网络与协议：基于 `epoll` LT 实现单线程事件循环，RESP2 解析器支持半包、粘包和 pipeline，一次读事件可连续解析多条完整命令。
- 存储与对象：实现 SDS、DICT、RedisObject 和 InMemoryDB；DICT 使用 Redis 风格双表渐进式 rehash，Hash 类型复用该底层结构。
- 持久化：AOF 覆盖写命令追加、启动 replay、同步 rewrite、后台 `BGREWRITEAOF`、rewrite buffer 和 `appendfsync` 策略。
- 主从复制：实现 `PING -> REPLCONF -> PSYNC` 握手、`FULLRESYNC` 命令流全量同步、写命令传播、backlog、partial resync 和 replica 断线重连。
- 工程验证：使用 GTest 覆盖基础结构、协议、配置、命令、AOF 和 TCP E2E；复制 E2E 覆盖全量同步、增量传播、backlog 补发和重连恢复。

## 复制流程
```text
replica 启动
-> connect master
-> PING
-> REPLCONF listening-port <port>
-> PSYNC ? -1
-> FULLRESYNC <replid> <offset>
-> 回放快照命令流
-> TINYREDIS-SNAPSHOT-END <offset>
-> 进入 streaming，持续接收 master 写命令

断线重连：
replica 使用已保存的 <replid, offset> 发送 PSYNC
-> master 判断 offset 是否仍在 backlog
-> 命中：CONTINUE + 补发 backlog 缺失命令
-> 未命中：退回 FULLRESYNC
```

## AOF 行为边界

当前 AOF 模块已经覆盖以下主链路：

- 写命令追加：仅成功执行的写命令才会追加到 AOF
- 启动恢复：按 RESP 命令流顺序回放，重建内存状态
- 重写：支持 `REWRITEAOF` 与 `BGREWRITEAOF`
- 刷盘策略：支持 `appendfsync always/everysec/no`

当前实现的容错/失败语义：

- 如果 AOF 文件尾部只有一条不完整命令，恢复时会保留前面已经成功回放的数据
- 如果 AOF 中出现明确的 RESP 格式错误，加载会失败
- 如果 AOF 中命令语义执行失败，例如对字符串 `"abc"` 执行 `INCR`，加载会失败
- 后台 rewrite 进行中再次发起 `REWRITEAOF/BGREWRITEAOF` 会返回错误

当前未覆盖的更完整能力：

- AOF 校验和
- 更细粒度的损坏修复策略
- RDB + AOF 混合持久化
- rewrite/backlog 级别的更深入性能优化

## 当前阶段

项目当前处于“核心链路已完成，进入工程化收口与下一阶段主线选择”的阶段。

- 已经具备可运行、可测试、可演示的最小 Redis 内核子集
- 短期重点不是继续零散堆命令，而是先补齐路线、能力矩阵和展示材料
- 下一阶段建议在 `List`、`RDB snapshot`、更多复制边界测试三条主线中只选一条推进


## 目录结构
```text
TinyRedis/
├── CMakeLists.txt
├── main.cpp
├── include/                    # 头文件
│   ├── command/                # 命令解析、分发、DB 接口
│   │   ├── commandDispatcher.hpp
│   │   ├── commandParser.hpp
│   │   └── inMemoryDB.hpp
│   ├── config/                 # 配置文件解析与服务配置结构
│   │   └── serverConfig.hpp
│   ├── core/                   # 基础数据结构
│   │   ├── dict.hpp
│   │   └── sds.hpp
│   ├── net/                    # 网络与事件循环
│   │   ├── clientSession.hpp
│   │   ├── epollServer.hpp
│   │   ├── masterReplicationLink.hpp
│   │   └── socketUtil.hpp
│   ├── object/                 # Redis 对象模型
│   │   └── redisObject.hpp
│   ├── persistentence/         # AOF 持久化模块
│   │   └── aof.hpp
│   ├── replication/            # 复制状态和协议辅助函数
│   │   ├── replicationProtocol.hpp
│   │   └── replicationState.hpp
│   └── protocol/               # RESP 协议编解码
│       ├── respEncoder.hpp
│       ├── respObject.hpp
│       └── respParser.hpp
├── src/                        # 源码实现
│   ├── command/
│   │   ├── commandDispatcher.cpp
│   │   ├── commandParser.cpp
│   │   └── inMemoryDB.cpp
│   ├── config/
│   │   └── serverConfig.cpp
│   ├── core/
│   │   ├── dict.cpp
│   │   └── sds.cpp
│   ├── net/
│   │   ├── epollServer.cpp
│   │   └── socketUtil.cpp
│   ├── object/
│   │   └── redisObject.cpp
│   ├── persistentence/
│   │   └── aof.cpp
│   └── protocol/
│       ├── respEncoder.cpp
│       └── respParser.cpp
├── test/                       # 单元测试
│   ├── test_aof.cpp
│   ├── test_command.cpp
│   ├── test_config.cpp
│   ├── test_dict.cpp
│   ├── test_e2e.cpp
│   ├── test_resp.cpp
│   └── test_sds.cpp
├── docs/                       # 设计文档与路线文档
│   ├── assets/
│   │   └── v0.1.png
│   ├── design.md
│   └── roadmap.md
├── conf/                       # 配置样例
│   └── tinyredis.conf
└── build/                      # CMake 构建目录（已在 .gitignore 中忽略）
```

## 快速开始
```bash
cmake -S . -B build
cmake --build build -j
./build/tinyredis
```

默认监听 `127.0.0.1:6379`，也可以通过第一个参数指定端口：

```bash
./build/tinyredis 6380
```

也可以使用配置文件：

```bash
./build/tinyredis --config conf/tinyredis.conf
./build/tinyredis --config conf/tinyredis.conf --port 6380
```

当前支持的配置项：

```conf
port 6379
appendonly yes
appendfilename appendonly.aof
appendfsync everysec
# replicaof 127.0.0.1 6379
```

## 运行测试
```bash
ctest --test-dir build --output-on-failure
```

也可以单独运行核心测试：

```bash
./build/test_dict
./build/test_command
./build/test_aof
./build/test_e2e
```

其中 `test_e2e` 会启动本机 TCP 服务进程，覆盖客户端命令流、异常连接、主从全量同步、partial resync 和重连同步。

## 简历描述参考
```text
TinyRedis：基于 C++17 实现 Redis 兼容内存数据库内核，使用 epoll 单线程事件循环处理 RESP2 pipeline 请求；实现 SDS、渐进式 rehash 字典、String/Hash/TTL 命令、AOF rewrite 和 PSYNC 主从复制，支持 replication backlog、partial resync 与断线重连，并通过 GTest/TCP E2E 覆盖协议、持久化和复制故障场景。
```

## 文档索引
- [设计说明](docs/design.md)
- [项目路线图](docs/roadmap.md)
- [性能基线](perf/README.md)
- 任务拆分与进度跟踪以 GitHub `Issues/Projects` 为主

## 性能摘要
当前已记录第一版性能基线，包括 AOF 策略对比和 no AOF 不同客户端数的并发基线。

详见：[perf/README.md](perf/README.md)

## 相关文章
[从0到1做一个 C++ Redis内核：项目设计与模块拆分](https://blog.csdn.net/2402_87224981/article/details/160474316)  
[基于epoll的单线程Reactor：Tinyredis的网络层实现](https://blog.csdn.net/2402_87224981/article/details/160557193)
