# TinyRedis Roadmap

本文档用于记录 TinyRedis 当前阶段、复习顺序和后续开发取舍。它不是 Redis 功能清单，而是服务于简历项目和面试复盘的工程路线图。

## 1. 当前阶段

TinyRedis 当前已经越过“能跑一个 KV 服务”的阶段，进入：

- 核心链路基本收口
- 简历亮点已经具备
- 需要把已有实现复盘成稳定表达

当前已经具备的能力：

- 单线程 `epoll` 网络模型
- RESP2 编解码、半包、粘包、pipeline
- 命令分发链路和统一错误处理
- SDS 动态字符串
- DICT 渐进式 rehash 哈希表
- String / Hash / TTL 命令
- AOF append、replay、rewrite、BGREWRITEAOF
- master/replica 复制、全量同步、写命令传播、backlog、partial resync、断线重连
- 单元测试和 TCP E2E 测试基线

现阶段最重要的事情不是继续堆命令，而是：

1. 把底层结构讲清楚：`SDS`、`DICT`
2. 把系统链路讲清楚：请求链路、AOF、Replication
3. 把测试看明白：哪些行为已经验证，哪些还只是功能实现
4. 把与 Redis 官方实现的差异讲清楚

## 2. 模块优先级

| 模块 | 当前状态 | 接下来重点 |
| --- | --- | --- |
| 网络 / 协议 / 分发 | 基础版完成 | 讲清 `SET k v` 从 socket 到响应写回的函数链路 |
| SDS | 底层亮点 | 讲清 `len/alloc/flags`、`buf_` 指针回退和 header 升级 |
| DICT | 底层亮点 | 讲清双表、链地址法、渐进式 rehash 和查询双表 |
| String / Hash / TTL | 基础版完成 | 不继续堆命令，先讲清对象模型和过期策略 |
| AOF | 功能闭环完成 | 讲清 append、fsync、rewrite、bgrewrite 和 rewrite buffer |
| Replication | 简历亮点 | 讲清握手、全量同步、写传播、backlog、partial resync、断线重连 |
| 测试 | 已有基线 | 系统阅读 `test_sds/test_dict/test_aof/test_e2e` |
| Redis 差异 | 需要整理 | 讲清 TinyRedis 保留了什么思想，又简化了什么生产级机制 |

## 3. 三天复习路线

### Day 1: 底层结构 + 测试

目标：

- 把 SDS 和 DICT 讲成自己的语言
- 把测试文件扫一遍，知道项目验证了什么

建议顺序：

1. `test_sds.cpp`
2. `test_dict.cpp`
3. `test_command.cpp`
4. `test_aof.cpp`
5. `test_e2e.cpp`

完成标准：

- 能说出每个测试文件主要验证哪些行为
- 能挑出 3 个最能体现工程质量的测试点

### Day 2: AOF + Replication

目标：

- 把两个系统模块讲顺
- 能落到关键函数和文件

建议顺序：

1. AOF：`appendCommand -> flushIfNeeded -> replay -> rewriteCommands -> startBackgroundRewrite -> pollBackgroundRewrite`
2. Replication：`initReplication -> connectMaster -> handleMasterRead -> fullResyncPayload -> propagateToReplicas`

完成标准：

- 能画出 AOF 四条链路
- 能画出复制握手、全量同步、写命令传播、断线重连四条链路

### Day 3: Redis 差异 + 简历表达

目标：

- 把项目从“代码会写”整理成“面试能讲”

建议准备：

- TinyRedis 和 Redis 在网络、数据结构、AOF、RDB、复制上的差异
- 项目的 3 个亮点
- 项目的 3 个不足
- 如果继续做，下一步为什么选 RDB 或复制 ACK

完成标准：

- 30 秒能讲项目定位
- 2 分钟能讲核心架构
- 5 分钟能讲 AOF 或 Replication
- 被追问不足时能主动说出 Redis 官方实现更完整在哪里

## 4. 简历优先级

当前最适合写进简历的点：

- C++17 实现 Redis 兼容内存数据库核心链路
- 基于单线程 `epoll` 实现 TCP 服务，支持 RESP2 pipeline、半包和粘包解析
- 实现 SDS 和渐进式 rehash 字典，支撑 String / Hash / TTL 存储
- 实现 AOF append、replay、rewrite、后台重写和多种 fsync 策略
- 实现简化 PSYNC 主从复制，支持 backlog、partial resync 和断线重连
- 通过 GTest 和 TCP E2E 测试覆盖协议、命令、AOF 和复制场景

不建议当前继续强调：

- “完整 Redis”
- “高性能超过 Redis”
- “支持分布式集群”

这些说法和当前实现不匹配，面试追问时风险较高。

## 5. 后续可选开发

如果复习完成后还想继续增强，建议只选一条主线。

优先级：

1. RDB 快照和基于 RDB 的全量复制
2. 复制 ACK、心跳、replid2 和故障切换历史
3. Hash 常用命令补全
4. `SET EX/PX/NX/XX`
5. benchmark 和性能报告整理

当前不建议优先做：

- 一次性补很多零散命令
- 同时展开多个新数据结构
- 过早做复杂 agent 包装
- 在复习没收口前大改架构

## 6. 一句话路线

`停止盲目加功能，把 SDS、DICT、AOF、Replication、测试和 Redis 差异讲透。`
