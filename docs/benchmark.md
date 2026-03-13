# MiniVecDB Benchmark 运行指南

## 1. 编译与数据准备
```bash
bazel build //tools:gen_dataset //tools:run_benchmark_flat //tools:run_benchmark_graph //tools:run_benchmark_hnsw
./bazel-bin/tools/gen_dataset 100000 128 test_100k.mvdb
```
## 2. 三方同台压测指令

统一输出 CSV 格式字段：

```
N, dim, Q, topk, M, efC, efS, build_ms, avg_ms, p95_ms, qps, recall@K
```

**Flat 真值基线：**

```Bash
./bazel-bin/tools/run_benchmark_flat test_100k.mvdb 1000 10
```

**GraphIndex (单层图)：**

```Bash
./bazel-bin/tools/run_benchmark_graph test_100k.mvdb 1000 10
```

**HnswIndex (分层小世界图，可扫参)：**
```bash
# 参数：M=16, efConstruction=200, efSearch=50
./bazel-bin/tools/run_benchmark_hnsw test_100k.mvdb 1000 10 16 200 50
```