# TinyRedis 设计说明

## 1. 分层

项目按层拆分为：

- `core`：基础数据结构（`SDS`、`DICT`）
- `protocol`：RESP 解析与编码
- `command`（计划中）：命令解析与分发
- `server`（计划中）：网络与事件循环

这样可以让协议层与存储层、网络层解耦，便于独立测试和演进。

## 2. 数据流（目标形态）

1. TCP 读取字节流
2. RESP 解析器构造 `RESPObject`
3. 命令层转换为 `argv`
4. 分发器在 DB 上执行命令
5. RESP 编码器回写响应

## 3. 核心结构

### SDS

- 管理字符串内存所有权。
- 记录 `len` 与 `alloc`。
- 支持 append 扩容策略。

### DICT

- 使用链式冲突处理的哈希表。
- 当前阶段：单活动表行为。
- 计划：实现 Redis 风格双表渐进 rehash。

## 4. 协议层

RESP2 支持顺序目标：

1. Simple String / Error / Integer / Bulk String / Array
2. Null 变体
3. 健壮的分包与半包处理

当前解析器为流式接口（`feed + parse`），适合 socket 持续读取场景。

## 5. 服务模型（计划）

- 先做单线程事件循环（贴近 Redis 的基础哲学）。
- 在协议与命令正确性稳定前，不引入并发复杂度。
- 后续只在基准测试证明有必要时再做热点优化。
