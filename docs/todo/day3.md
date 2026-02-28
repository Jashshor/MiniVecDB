你现在的进度非常到位：**工程基建（Bazel/CI/format）+ VectorStore（连续内存/零拷贝）+ IIndex 接口**都已经落地。
所以 Day3 的唯一主线目标是：

> **交付“可对照的检索基线”：FlatIndex（暴力检索）端到端跑通 + 正确性测试 + 基础性能基线。**
> 这是后面 HNSW/ANN 的“真值”和“对照组”，没有 FlatIndex，后面所有优化都无法证明价值。

### Day3 交付物（必须落地）

1. `index::FlatIndex` 实现（支持 `Build + Search`）
2. 与 VectorStore 集成的端到端 Demo/CLI 路径（至少能：加载数据→build→search）
3. 单测覆盖 FlatIndex 正确性（包括边界情况）
4. 一个最简 benchmark（QPS + p95 latency，先不用 recall）

### 范围控制（防止 Day3 失焦）

- **不做并发**（单线程先稳定）
- **不做多度量扩展**（先 L2 跑通，Cosine/IP 以后再抽象）
- **不做复杂数据格式**（你现在的 raw binary 可以继续用，但要把“读取/验证”做实）

------

## Part 2｜助手给你的 Day3 任务清单（全面、可验收、按优先级拆解）

下面是 Day3 的任务拆解（P0 必须做完；P1 建议做；P2 加分）。

------

# ✅ P0（必须完成）｜FlatIndex 基线 + 正确性 + 端到端

## 1) 实现 `index::FlatIndex`（核心）

### 建议设计（最省事且工程正确）

- `FlatIndex` **不持有向量内存**，只持有对 `VectorStore` 的指针/引用（或内部 idx 列表）
- `Build(store)`：记录 `store_ = &store`，可选记录 `dim_`、`size_`
- `Search(query, dim, topk)`：
  - 遍历 `[0..store->Size())`
  - 调 `metric::l2_distance(query, store->GetVectorPtr(i), dim_)`
  - 用 **固定大小 topK 堆** 保留最优结果

### TopK 堆建议（关键点）

- L2：距离越小越好
- 使用 `std::priority_queue<Neighbor>` 让 “**当前最差的候选**”在堆顶（方便淘汰）
- `Neighbor` 的比较逻辑要保证：
  - 堆顶是距离最大的那个（最差）
  - 距离相同最好做稳定 tie-break（比如 idx 小的优先），否则测试容易抖

> 验收：`bazel test //...` 仍然全绿，并新增 flatindex 测试。

------

## 2) FlatIndex 单元测试（至少 5 个）

建议写 `tests/test_flat_index.cpp`，覆盖：

1. **SmallExactMatch**
   - 手工构造 5 条 dim=2/3 向量 + 1 条 query
   - 期望 top1/topk 结果完全一致（你可以手算距离）
2. **TopKEqualsN / TopKGreaterThanN**
   - topk >= N 时，返回 N 条，且按距离排序正确
3. **DimMismatchRejected**
   - query dim 不对：明确失败行为（返回空/抛异常/Status，按你工程现状来，但要一致且可测）
4. **DeterministicOrderOnTie（建议）**
   - 构造相同距离的向量，验证 tie-break 规则稳定
5. **MatchesBruteforceTruth（本质回归）**
   - 用同一批向量，flat 的结果应该与 “全量排序”一致（你在测试里直接排序一遍当真值）

> 验收：这些测试全通过，且可重复运行不抖动。

------

## 3) 数据加载器：把 `.bin` 数据喂给 VectorStore

你已经能生成 raw binary 了，但 Day3 必须把它“用起来”。
建议新增 `tools/load_dataset.cpp` 或者在 `vecdb_cli` 里加一个 loader：

### 约定一个清晰规则（今天就定死）

- 数据文件只包含 `N * dim` 个 `float`（无 header）
- id 规则：默认 `id = 0..N-1`
- 加载时必须校验：文件大小是否整除 `dim*sizeof(float)`，否则报错

> 验收：`vecdb_cli --load xxx.bin --dim 128` 能成功插入 N 条，并能 `--search`。

------

## 4) 端到端 CLI/Demo（用 FlatIndex 串起来）

`bazel run //src:vecdb_cli` 至少支持这一条路径：

- `--dim 128 --data data.bin --topk 10 --query_mode random`
  - 加载数据 → store.reserve(N) → 插入 → build flat → 取随机向量当 query 或随机生成 query → search → 打印 topk（id + dist）+ 耗时

> 验收：可以在 10k/100k 数据规模跑起来，并输出合理耗时。

------

# ✅ P1（强烈建议）｜最简 Benchmark：给未来 HNSW 对照用

## 5) `tools/run_benchmark.cpp`

输入参数建议：

- `--data data.bin --dim 128 --q 1000 --topk 10`
- query 来源（选一个就行）：
  - `random_from_dataset`（从数据集中抽 q 条向量）
  - 或 `random_generate`

输出指标（Day3 先输出最小集合）：

- 平均 latency（ms）
- p95 latency（ms）
- QPS（q / total_time）

> 验收：benchmark 可复现、可重复运行，输出稳定。

------

# ✅ P2（加分项）｜为 Day4 持久化铺路（只做接口/占位即可）

## 6) 为 “持久化 Save/Load” 提前定接口（不实现也行）

比如：

- `storage::Serializer::SaveVectors(path, store)`
- `storage::Serializer::LoadVectors(path) -> VectorStore`
  今天只要把接口和文件 layout 规划写到 `docs/design.md` 的一个小节即可。

------

# Day3 时间盒建议（不加班版）

- **1/2 天**：FlatIndex 实现 + 单测
- **1/4 天**：bin loader + CLI 跑通
- **1/4 天**：benchmark 工具 + 指标输出

------

# Day3 最终验收标准（你对照自检即可）

你今晚给我汇报时，只要满足这 5 条，Day3 就算高质量完成：

1. `index::FlatIndex`（Build+Search）已实现 ✅
2. `tests/test_flat_index.cpp` 至少 5 个用例，全绿 ✅
3. `vecdb_cli` 可加载 `.bin`，可 search topK ✅
4. `run_benchmark` 输出 avg/p95/QPS ✅
5. README 更新：新增 CLI/benchmark 使用命令 ✅

------

## 提交建议（保持你现在的原子性节奏）

- 分支：`day3-flat-index`
- commit 拆法（推荐 3～4 个）：
  1. `feat(index): implement FlatIndex brute-force search`
  2. `test(index): add FlatIndex correctness tests`
  3. `feat(tools): add dataset loader and cli search demo`
  4. `feat(bench): add baseline benchmark for FlatIndex`