## Part 2｜按我的路线重新排：Day5 做地基，Day6 上 HNSW 主线

# ✅ Day5（今天）目标：为 ANN 做好“可衡量、可复现、可维护”的地基

## P0（必须完成）

1. **MVDB v1 读写加固（协议工程化）**

* 明确协议端序：**little-endian**
* 不再依赖 `packed struct` 的“直接 read/write 内存布局”

  * 改为字段级序列化（或 buffer + memcpy 到对齐变量）
* 加入溢出/长度校验：`count * dim`、文件长度、truncated 检测
* 文档补齐：design.md 写清字段顺序/类型/端序/版本策略

✅验收：原 serializer 测试全绿 + 新增 1 个“header 解析正确性”测试

2. **Benchmark 增加 recall@K（ANN 的生命线）**

* 用 FlatIndex 生成 ground-truth topK
* 被测索引（先仍是 FlatIndex）跑 topK
* 计算 `recall@K = |intersection| / K`
* 输出：avg / p95 / qps / recall@K

✅验收：Flat vs Flat 的 recall@K = 1.0 且输出稳定

## P1（强烈建议）

3. **先做单层图 ANN（NSW/GraphIndex 骨架）**

* Build：顺序插入 + 搜索近邻 + 截断到 M + 近似无向边
* Search：best-first + visited + topK
* 能通过 benchmark 看见：调大 `ef_search` recall 上升、延迟上升

✅验收：小数据集跑通，recall@10 可量化、可调参

## P2（加分项）

4. **统一错误处理风格（别混用 throw/bool/status）**

* 选一种全局策略并逐步收敛（后面服务化会省很多麻烦）
