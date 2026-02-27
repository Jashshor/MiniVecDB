## Part 1｜组长给你的 Day 2 任务清单（实际执行版）

> **Day 2 目标**：把 `VectorStore` 从“接口草案”落成“可用模块”，用测试把它钉死；并完成纯二进制数据生成工具，为 Day 3 的 `FlatIndex` 提供“零拷贝读取”能力与压测弹药。

✅ **P0（必须完成，完成即合格）**

**1) 实现 `storage::VectorStore`（.cpp 落地）**

- **内存布局**：
  - `size_t dim_`：维度。
  - `std::vector<float> data_`：按行连续存储的一维大数组（Row-major，彻底扁平化）。
  - `std::vector<uint64_t> ids_`：内部 idx -> 外部 id 映射。
  - `std::unordered_map<uint64_t, uint32_t> id_to_idx_`：外部 id -> 内部 idx 映射。
- **核心接口**：
  - `void reserve(size_t n)`：预分配内存，避免插入时扩容拷贝。
  - `uint32_t add(uint64_t id, const float* vec, size_t dim)`：维度不匹配或 ID 重复直接抛出异常，数据通过指针拷贝至数组尾部。
  - `const float* get_vector(uint32_t internal_idx) const`：基于内部偏移量，返回大数组切片的**零拷贝裸指针**。
  - `uint64_t get_id(uint32_t internal_idx) const`：反查 ID。
  - `size()` 与 `dim()`。

**2) 补齐 VectorStore 单元测试（GTest）**

- 编写 `tests/test_vector_store.cpp`，覆盖 4 个核心用例：
  - `AddAndGetWorks`：插入数据后，验证零拷贝返回的指针内容正确。
  - `RejectDuplicateId`：重复 ID 插入抛出异常。
  - `RejectDimMismatch`：维度不匹配抛出异常。
  - `IdMappingCorrect`：外部 ID 与内部偏移量映射一致。
- 配置 `tests/BUILD.bazel`，执行 `bazel test //...` 全绿通过。

**3) 完善 VectorStore 使用示例（Demo 落地）**

- 修改 `src/main.cpp`：
  - 创建 `VectorStore`（dim=4），执行 `reserve(100)`。
  - 插入 2 条测试向量。
  - 零拷贝读取并打印其中一条向量的具体数值，结合 `Timer` 输出总耗时。
- 执行 `bazel run //src:vecdb_cli` 验证输出。

✅ **P1（建议完成，为计算层做准备）**

**4) 给 FlatIndex 做接口准备**

- 修改 `src/index/iindex.h`：
  - 定义统一返回结构体 `Neighbor`（含 `internal_idx` 与 `distance`），重载 `operator<` 以支持堆排序。
  - 定义多态接口 `build(const storage::VectorStore& store)` 与 `search(const float* query, size_t dim, size_t topk)`。

✅ **P2（加分项，为基准评测做准备）**

**5) 开发批量数据集生成工具**

- 编写 `tools/gen_dataset.cpp`：
  - 支持解析命令行参数 `<N> <dim> <output_file>`。
  - 采用**纯二进制格式（Raw Binary）**落盘，拒绝使用 Protobuf 以保证后续极速 I/O。先写入 Header (N, dim)，再连续写入 float 随机数。
- 配置 `tools/BUILD.bazel` 的 `cc_binary` 目标。
- 解决 Bazel 沙盒路径隔离问题，通过编译后执行 `./bazel-bin/tools/gen_dataset 10000 128 dummy_data.bin` 将文件生成在实际工作区中。

------

## Part 2｜执行版 Checklist（按小时推进）

**0）启动（5 分钟）**

- [x] 确认当前在 `day2-vectorstore` 分支上，工作区干净。

**1）存储核心落地（40–60 分钟）**

- [x] 完善 `src/storage/vector_store.h`，定义扁平化数据结构与 `const float*` 接口。
- [x] 新建 `src/storage/vector_store.cpp`，实现 `reserve`、`add`、`get_vector` 等逻辑。
- [x] 更新 `src/storage/BUILD.bazel` 加入 `.cpp` 源文件。

**2）单元测试与验证（30–50 分钟）**

- [x] 新建 `tests/test_vector_store.cpp`，写满 4 个边界测试用例。
- [x] 在 `tests/BUILD.bazel` 声明测试目标并链接 `vector_store` 依赖。
- [x] 盲敲 `bazel test //...`，修改直至全绿通过。

**3）检索接口与 Demo 串联（20–30 分钟）**

- [x] 更新 `src/index/iindex.h`，增加 `Neighbor` 结构体及搜索虚函数。
- [x] 更新 `src/main.cpp`，串联 Timer 和 VectorStore，编译并运行 `bazel run //src:vecdb_cli` 确认终端输出符合预期。

**4）批量数据生成工具（30–40 分钟）**

- [x] 编写 `tools/gen_dataset.cpp`，使用 `fstream` 进行 C++ 底层二进制写文件操作。
- [x] 配置 `tools/BUILD.bazel`。
- [x] 验证运行：`bazel build //tools:gen_dataset` 后运行二进制文件，确认当前目录下成功生成 `.bin` 数据文件。

**5）规范化提交与清理战场（10 分钟）**

- [x] `git add src/` -> `git commit -m "feat(storage): implement VectorStore contiguous layout"`
- [x] `git add tests/ tools/` -> `git commit -m "test(storage): add VectorStore unit tests and dataset tool"`
- [x] `git checkout main` -> `git merge day2-vectorstore` -> `git push origin main`
- [x] `git checkout -b day3-flat-index`

------

## 今日验收结果

- [x] `bazel test //...` 全绿（涵盖存储逻辑的核心边界）。
- [x] `VectorStore` 可正常分配连续内存，`get_vector` 返回零拷贝指针。
- [x] `vecdb_cli` Demo 运行成功并打印准确的执行耗时。
- [x] `.bin` 测试数据文件在本地成功生成。
- [x] 代码以原子级 Commit 合入 `main` 分支，当前停留在干净的 `day3-flat-index` 研发分支。