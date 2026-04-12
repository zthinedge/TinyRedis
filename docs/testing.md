# 测试策略

## 测试层级

- 单元测试：`SDS`、`DICT`、`RESP`。
- 集成测试（计划中）：命令执行链路。
- 端到端测试（计划中）：通过 TCP socket 验证整体行为。

## 当前常用命令

构建：

```bash
cmake -S . -B build
cmake --build build -j4
```

运行测试：

```bash
cd build && ctest --output-on-failure
```

## 覆盖重点

### SDS

- 构造、append、clear、移动语义。
- 大 payload 下的扩容行为。

### DICT

- set/get/erase 正确性。
- 已存在 key 的更新行为。
- rehash 实现后的行为验证。

### RESP

- 基础类型编码与解码。
- 分包/半包解析。
- 单缓冲区多消息连续解析。
- 非法协议输入处理。

## 完成定义（Done Definition）

一个功能仅在以下条件满足时视为完成：

- 有测试覆盖。
- 本地测试通过。
- 若涉及非平凡取舍，已记录到 `docs/decisions.md`。
