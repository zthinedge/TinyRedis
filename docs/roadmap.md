# TinyRedis Roadmap

## 目标

- 实现一个可运行、可测试、可演进的 Redis 核心子集。
- 优先保证正确性，再考虑性能和高级特性。

## 当前基线

- 网络：单线程 `epoll` LT 事件循环。
- 协议：RESP2 基础解析与编码。
- 命令：`PING/SET/MSET/GET/MGET/DEL/EXISTS/INCR/INCRBY/DECR/EXPIRE/TTL/PTTL/PERSIST`。
- 过期：惰性过期 + cron 主动抽样清理。
- 持久化：AOF，支持追加写入、启动 replay 和同步 rewrite。
- 测试：`test_sds`、`test_dict`、`test_resp`、`test_command`、`test_aof`、`test_e2e`。

## Now

- 补齐命令边界测试和错误返回测试。
- 增强 RESP 半包、粘包、非法输入测试。
- 保持 README、设计文档和代码结构一致。
- 默认保证 `ctest --output-on-failure` 通过。

## Next

- 完善 AOF 错误处理和刷盘策略。
- 增加 AOF rewrite 触发策略与配置项。
- 补齐 String 常用命令选项。
- 优化 `InMemoryDB` 与命令层的接口边界。

## Later

- 支持 List / Hash / Set / ZSet 核心子集。
- 增加基础指标、慢命令或调试信息。
- 建立吞吐和延迟基线。
- 对比 LT / ET 网络模型。

## 完成标准

- 功能有测试。
- CTest 通过。
- 行为和文档一致。
- 关键变更同步更新 README / docs。
