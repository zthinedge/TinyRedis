# TinyRedis 设计说明

## 1. 分层架构

```text
Client
  |
  v
net            EpollServer / ClientSession
  |
  v
protocol       RESPParser / RESPEncoder / RESPObject
  |
  v
command        CommandParser / CommandDispatcher
  |
  v
storage        InMemoryDB / RedisObject
  |
  v
core           SDS / DICT
```

旁路模块：

- `persistentence/AOF`：写命令追加、启动恢复与同步重写。
- `cron`：事件循环中的周期任务，当前不是独立线程。

## 2. 主请求链路

```text
Client
-> EpollServer::handleClientRead
-> RESPParser::feed / parse
-> CommandParser::toArgv
-> CommandDispatcher::dispatch
-> CommandDispatcher::dispatchInternal
-> InMemoryDB
-> RESPEncoder
-> ClientSession::writeBuf
-> EpollServer::handleClientWrite
-> Client
```

核心职责：

- `EpollServer`：负责 `epoll`、`accept`、`recv`、`send`。
- `RESPParser`：负责请求字节流解析，内部维护读缓冲。
- `CommandParser`：把 `RESPObject` 转成 `argv`。
- `CommandDispatcher`：识别命令、检查参数、调用 DB、管理 AOF。
- `InMemoryDB`：保存 KV 数据和 TTL 元信息。
- `RESPEncoder`：生成 Redis RESP 响应。

## 3. ClientSession

```text
ClientSession
├── RESPParser parser
├── std::string writeBuf
└── bool closeAfterWrite
```

- 没有单独 `readBuf`：读缓冲在 `RESPParser` 内部，用于处理半包/粘包。
- `writeBuf` 放在 session 中：非阻塞 `send` 可能一次发不完，需要保存剩余响应。

## 4. AOF

当前网络服务默认开启 AOF，默认文件为运行目录下的 `appendonly.aof`。AOF 文件不存在时，启动恢复按空库处理。

运行时写入：

```text
写命令
-> dispatchInternal(argv, false)
-> InMemoryDB
-> AOF::appendCommand
```

写入规则：

- 参与 AOF 的写命令：`SET/MSET/DEL/INCR/INCRBY/DECR/EXPIRE/PERSIST`。
- 命令参数按 RESP array 编码落盘，复用协议层编码格式。
- 当前实现每条写命令追加后同步 `fsync`。
- 当前命令链路先修改内存 DB，再追加 AOF；如果追加失败，会向客户端返回 AOF 错误，内存状态不回滚。

启动恢复：

```text
EpollServer::init
-> CommandDispatcher::loadAof
-> AOF::replay
-> dispatchInternal(argv, true)
-> InMemoryDB
```

`dispatchInternal(argv, true)` 表示当前在回放 AOF，不会再次追加 AOF，避免重启后重复写入。

同步重写：

```text
REWRITEAOF / BGREWRITEAOF
-> InMemoryDB::snapshot
-> 生成 SET / EXPIRE 恢复命令
-> AOF::rewriteCommands
```

重写规则：

- `REWRITEAOF` 与 `BGREWRITEAOF` 当前都走同步重写路径。
- 重写基于 `InMemoryDB::snapshot` 生成恢复命令，只保留当前仍有效的 key。
- 每个 key 先生成 `SET key value`；如果 key 有剩余 TTL，再追加 `EXPIRE key seconds`。
- `AOF::rewriteCommands` 先写入 `appendonly.aof.tmp`，成功 `fsync` 后再原子替换目标文件。

## 5. 过期与 cron

- 惰性过期：访问 key 时检查 TTL。
- 主动过期：`EpollServer::run` 中周期触发 `CommandDispatcher::cron`。
- cron 当前运行在线程事件循环中，不是独立线程。

```text
EpollServer::run
-> CommandDispatcher::cron
-> InMemoryDB::activeExpireCycle
```
