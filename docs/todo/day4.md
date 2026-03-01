> **持久化（Save/Load）落地 + 端到端冷启动链路跑通 + 数据格式稳定化（可版本演进）**

为什么 Day4 必须做持久化：
后面做 HNSW/ANN 时，你需要反复对比实验（不同参数、不同数据集、不同版本），如果每次都从 `.bin` 重灌并 build，会极度浪费时间，也很难复现结果。持久化是“工程进入可迭代状态”的关键。

### Day4 交付物（必须落地）

1. `storage::Serializer`：完成 **VectorStore 的 Save/Load**（严格按你 design.md 的 layout）
2. CLI 打通冷启动：**load store → build FlatIndex → search / benchmark**
3. 测试闭环：**Round-trip 一致性 + 文件损坏/异常处理**
4. 数据格式统一：`gen_dataset` / loader / serializer **对同一格式达成一致**（避免出现两套 `.bin` 语义）

### 范围控制（Day4 不做）

* 不持久化 FlatIndex / HNSW Index（先只持久化 VectorStore）
* 不做 WAL、增量 checkpoint（你可以在 design 写计划，但今天不实现）
* 不搞 mmap/零拷贝文件映射（可加分，但不是主线）

---

## Part 2｜助手给你的 Day4 任务清单（全面拆解 + 验收标准 + 提交建议）

下面按 P0/P1/P2 列清单，你照着做就不会跑偏。

---

# ✅ P0（必须完成）｜Serializer 落地 + 冷启动链路 + 严格测试

## 1) 统一并“冻结”二进制文件格式（今天必须定死）

你在 `docs/design.md` 已经规划了 layout，Day4 要把它变成 **代码里的单一事实**。

**推荐最小 Header（versioned, fixed-width, little-endian）**：

* `magic[4] = "MVDB"`（或你已定义的）
* `uint32_t version = 1`
* `uint32_t dim`
* `uint64_t count`
* `uint64_t reserved`（对齐/扩展用）
* 然后：

  * `ids[count]`：`uint64_t`
  * `vectors[count * dim]`：`float`（IEEE754）

> 验收：`Serializer` 只认这一套格式；loader/gen_dataset/serializer 都一致。
> 你 Day3 写了“解析 .bin Header”，那 Day4 的重点就是：把这个 header 真正工程化（校验/版本/错误处理）。

---

## 2) 实现 `storage::Serializer`（Save/Load）

建议新增：

* `src/storage/serializer.h/.cc`

接口建议：

* `static void Save(const std::string& path, const VectorStore& store);`
* `static VectorStore Load(const std::string& path);`

实现要点（今天必须做到）：

* **完整校验**：

  * magic 不对 → fail
  * version 不支持 → fail
  * 文件大小不足（truncated）→ fail
  * `count*dim` 溢出检查（防止 size_t overflow）
* **加载策略**：

  * 先读 header
  * 构造 `VectorStore(dim)`，`Reserve(count)`
  * 读 ids 与 vectors（可以一次性读大块）
  * 用 VectorStore 的插入接口落数据（或提供一个内部“bulk load”接口更快）

> 验收：保存后文件可被 Load 正确恢复，恢复后 flat search 结果一致（见测试项）。

---

## 3) Round-trip 一致性测试（强制）

新增 `tests/test_serializer.cpp`，至少 4 个测试：

1. **SaveLoadRoundTripBasic**

   * 构造 store（dim=4，插入 3 条固定向量 + 固定 id）
   * Save → Load
   * 断言：dim、size、id 顺序/映射一致；向量逐元素相等（float 用 EXPECT_FLOAT_EQ）

2. **RejectBadMagic**

   * 写一个 magic 错误的文件（或保存后手动改 1 字节）
   * Load 应失败（异常/返回空/Status，按你工程风格统一）

3. **RejectUnsupportedVersion**

   * version 写成 999
   * Load 应失败

4. **RejectTruncatedFile**

   * 只写 header 不写 payload
   * Load 应失败

> 验收：`bazel test //...` 全绿，且这些测试多跑几次不抖。

---

## 4) CLI 冷启动链路打通（真实工程链路）

Day4 CLI 至少支持这个路径（你可以用 flag 组合）：

* `--load_store xxx.mvdb --build flat --search_random 10 --topk 10`
* 或：
* `--load_store xxx.mvdb --bench --q 1000 --topk 10`

建议新增两条能力：

* `--save_store out.mvdb`：把当前 store 保存
* `--load_store in.mvdb`：从磁盘加载 store（替代 Day3 的 loader）

> 验收：你可以做到“今天生成数据 → 保存 → 明天直接 load → benchmark”，不需要再重新灌数据逻辑。

---

# ✅ P1（建议完成）｜数据集工具链统一（避免两套 .bin 语义）

## 5) 统一 `tools/gen_dataset` 与 Serializer 格式

你目前 gen_dataset 生成的是 raw binary（Day2 汇报），但 Day3 又提到 loader 解析 header。Day4 必须收敛成 **一种格式**，建议直接让 gen_dataset 输出 **MVDB v1 格式**：

* `gen_dataset --n 100000 --dim 128 --out data.mvdb --seed 42`
* 写 header + ids + vectors

> 验收：`gen_dataset` 产物可直接被 CLI `--load_store` 读取，不需要额外转换。

---

## 6) benchmark 改造：支持直接用持久化数据

让 `tools/run_benchmark` 支持：

* `--load_store data.mvdb`（优先）
* 可选仍支持 `--load_raw_bin`（如果你想兼容旧格式，但不推荐继续扩散）

> 验收：压测工具从“每次灌数据”变成“直接加载 store”，省时间且复现更稳定。

---

# ✅ P2（加分项）｜为 Day5/Day6 的 ANN 做准备

## 7) 在 design.md 追加“索引持久化策略草案”（不实现）

写清楚两件事就够：

* 索引文件 `index.hnsw` / `index.flat` 的 layout（header + graph）
* 冷启动策略：Load store → Load index（如果版本匹配）否则 rebuild

> 加分原因：你后面做 HNSW 时不会临时拍脑袋改格式。

---

# Day4 时间盒建议（不爆炸版）

* 60%：Serializer Save/Load + 错误处理
* 25%：测试（round-trip + bad cases）
* 15%：CLI/benchmark 接入（冷启动链路）

---

# Day4 最终验收标准（你下班前自检）

满足这 5 条我就直接给你 Day4 通过：

1. `Serializer::Save/Load` 可用且严格校验 ✅
2. `test_serializer.cpp` ≥ 4 个用例，全绿 ✅
3. CLI 能 `--save_store` / `--load_store` 并串起 flat search ✅
4. gen_dataset 输出与 serializer 格式一致 ✅
5. benchmark 能直接 `--load_store` 跑出 avg/p95/QPS ✅

---

# 提交建议（保持你现在的原子提交风格）

分支：`day4-persistence`

推荐 3～4 个 commit：

1. `feat(storage): add Serializer for MVDB v1 format`
2. `test(storage): add serializer round-trip and validation tests`
3. `feat(cli): support save/load store cold start`
4. `feat(tools): update gen_dataset and benchmark to use mvdb format`

