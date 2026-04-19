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

- `persistentence/AOF`：写命令追加与启动恢复。
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

运行时写入：

```text
写命令
-> dispatchInternal(argv, false)
-> InMemoryDB
-> AOF::appendCommand
```

启动恢复：

```text
EpollServer::init
-> CommandDispatcher::loadAof
-> AOF::replay
-> dispatchInternal(argv, true)
-> InMemoryDB
```

`dispatchInternal(argv, true)` 表示当前在回放 AOF，不会再次追加 AOF，避免重启后重复写入。

## 5. 过期与 cron

- 惰性过期：访问 key 时检查 TTL。
- 主动过期：`EpollServer::run` 中周期触发 `CommandDispatcher::cron`。
- cron 当前运行在线程事件循环中，不是独立线程。

```text
EpollServer::run
-> CommandDispatcher::cron
-> InMemoryDB::activeExpireCycle
```
