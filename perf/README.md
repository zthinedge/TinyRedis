# TinyRedis 性能基线

这个目录用于保存 TinyRedis 的性能测试配置和本地 benchmark 脚本，方便后续重复测试和对比优化前后的性能变化。

## 1. 构建项目

```bash
cmake --build build -j
```

## 2. 启动 TinyRedis

纯内存基线，关闭 AOF：

```bash
./build/tinyredis --config perf/noaof.conf
```

开启 AOF，使用 `appendfsync everysec`：

```bash
./build/tinyredis --config perf/aof-everysec.conf
```

开启 AOF，使用 `appendfsync always`：

```bash
./build/tinyredis --config perf/aof-always.conf
```

## 3. 运行 Benchmark

另开一个终端执行：

```bash
bash perf/benchmark.sh
```

默认参数：

```text
HOST=127.0.0.1
PORT=6380
REQUESTS=100000
CLIENTS=50
TESTS=set,get,incr
```

也可以通过环境变量覆盖：

```bash
REQUESTS=200000 CLIENTS=100 TESTS=set,get,incr bash perf/benchmark.sh
```

脚本底层使用 `redis-benchmark`，默认只测试 TinyRedis 当前已经支持的命令，避免把未实现的 Redis 命令计入性能结果。

## 4. 第一版性能基线

### 4.1 AOF 策略对比

测试参数：

```text
requests=100000
clients=50
tests=set,get,incr
```

测试结果：

| 模式 | SET req/s | GET req/s | INCR req/s |
| --- | ---: | ---: | ---: |
| no AOF | 138504.16 | 161550.89 | 147929.00 |
| AOF everysec | 65487.89 | 161290.33 | 72727.27 |
| AOF always | 2788.00 | 160513.64 | 2786.68 |

结论：

- `GET` 是读命令，不写 AOF，因此三种模式下吞吐基本一致。
- `SET` 和 `INCR` 是写命令，开启 AOF 后吞吐下降明显。
- `appendfsync everysec` 相比 no AOF 有明显写入开销，但仍保持较高吞吐。
- `appendfsync always` 每条写命令都执行 `fsync`，写入吞吐大幅下降。

备注：

`redis-benchmark` 启动时会尝试执行 `CONFIG` 命令。TinyRedis 当前未实现 `CONFIG`，因此会输出类似如下 warning：

```text
ERROR: ERR unknown command 'CONFIG'
ERROR: failed to fetch CONFIG from 127.0.0.1:6380
WARN: could not fetch server CONFIG
```

该 warning 不影响 `set/get/incr` 的 benchmark 结果。

### 4.2 no AOF 并发基线

后续优化主要使用 no AOF 并发基线作为网络层和命令执行链路的对照。

测试参数：

```text
requests=100000
tests=set,get,incr
```

| clients | SET req/s | GET req/s | INCR req/s |
| ---: | ---: | ---: | ---: |
| 1 | 16482.61 | 16390.76 | 16688.92 |
| 10 | 113378.68 | 130890.05 | 108225.10 |
| 50 | 140845.08 | 166112.95 | 152207.00 |
| 100 | 145137.88 | 162601.62 | 153609.83 |
| 200 | 150602.42 | 168634.06 | 160000.00 |
