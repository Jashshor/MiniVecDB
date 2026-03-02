# MiniVecDB 架构与设计文档

本文档记录了 MiniVecDB 的核心架构决策、数据持久化协议（MVDB v1）以及未来的扩展规划。

## 1. 系统架构概览

MiniVecDB 采用**存储与计算分离**的极简架构，核心由两大部分组成：

* **Storage 层 (`storage::VectorStore`)**：负责最基础的原始向量数据（Raw Vectors）与外部 ID 的映射和存储。数据在内存中表现为极其紧凑的**一维扁平化大数组（Row-major）**，提供极致的 Cache 命中率和零拷贝（Zero-Copy）访问能力。
* **Index 层 (`index::IIndex`)**：计算与检索的核心。所有的索引（如 `FlatIndex`、未来的 `GraphIndex` 和 `HnswIndex`）都不实际持有向量数据本身，而是持有指向 `VectorStore` 的指针。这保证了无论构建多少种索引，原始向量数据在内存中只有一份副本。

## 2. 数据序列化协议 (MVDB v1 Format)

为了支持极速冷启动并保证跨平台安全，MiniVecDB 设计了严格的二进制持久化格式。

### 2.1 协议核心约束

* **端序 (Endianness)**：**严格约束为小端序 (Little-Endian)。** 在序列化与反序列化时，代码级强制校验当前机器端序，拒绝跨端序的错误加载。
* **对齐安全 (Alignment Safe)**：不依赖编译器的 `#pragma pack(1)` 魔法，在设计上保证了 Header 的 32 字节完美自然对齐，在读写时采用**逐字段序列化 (Field-by-field Serialization)**，彻底杜绝在 ARM 等架构上的 `SIGBUS` 未对齐访问崩溃异常。
* **浮点数标准**：向量数据严格遵循 32-bit IEEE 754 单精度浮点数标准 (`float`)。

### 2.2 文件内存布局 (Byte Layout)

完整的 `.mvdb` 数据文件由 **定长 Header (32 Bytes)** 和 **连续 Payload** 两部分拼接而成。

| 区域 | 字段名称 | 类型 | 字节大小 | 绝对偏移量 | 说明 / 约束 |
| --- | --- | --- | --- | --- | --- |
| **Header** | `magic` | `char[4]` | 4 bytes | `0x00` | 魔数签名，固定为 `'M', 'V', 'D', 'B'`。防呆设计。 |
|  | `version` | `uint32_t` | 4 bytes | `0x04` | 协议版本，当前固定为 `1`。用于向后兼容。 |
|  | `dim` | `uint32_t` | 4 bytes | `0x08` | 向量维度。 |
|  | `reserved1` | `uint32_t` | 4 bytes | `0x0C` | 显式对齐填充 (Padding)，固定填 `0`。凑齐 16 字节。 |
|  | `count` | `uint64_t` | 8 bytes | `0x10` | 当前文件包含的向量总条数。用于极速 `reserve`。 |
|  | `reserved2` | `uint64_t` | 8 bytes | `0x18` | 预留扩展字段，固定填 `0`。完美凑齐 32 字节对齐。 |
| **Payload** | `ids_array` | `uint64_t[]` | `count * 8` | `0x20` | 外部业务真实 ID 连续数组。 |
|  | `vectors_data` | `float[]` | `count * dim * 4` | `0x20 + count*8` | 扁平化的纯 float 向量连续数组 (Row-major)。 |

## 3. 错误处理模型 (Error Handling)

为保持系统早期迭代的简洁性与错误抛出的确定性，当前版本采用 **C++ 标准异常 (Exceptions)** 机制：

* `std::invalid_argument`：用于输入参数错误（如：维度不匹配）。
* `std::runtime_error`：用于执行期严重错误（如：重复 ID 插入、文件打开失败、文件 Header 损坏、魔数错误、版本不支持、机器端序不匹配等）。
* `std::out_of_range`：用于内部索引越界访问。

*(未来规划：如果向 RPC 微服务化演进，将在网络层接入前拦截异常，或重构为 `Status` / `Result<T, E>` 模式。)*

## 4. 索引层演进路线

### 4.1 当前支持

* **`FlatIndex`**：基于暴力全量扫描（Brute-force）的精确检索。使用大顶堆（Max-Heap）结合稳定的 Tie-breaker 规则维护 TopK 结果，作为评测其它近似检索算法召回率 (Recall) 的绝对真值基线 (Ground Truth)。

### 4.2 近期规划 (Day 5 - Day 10)

* **`GraphIndex` (NSW 单层图)**：基于贪心 Best-First Search 的单层导航小世界图算法，为多层架构探路。
* **`HnswIndex` (分层导航小世界图)**：引入多层概率跳表结构与启发式边裁减 (Heuristic Edge Selection)，实现高并发下的大规模亚毫秒级检索。

## 5. 索引持久化策略草案 (Draft for Future)

当引入极其消耗构建时间的图索引（如 HNSW）后，除了持久化 `VectorStore`，我们还需要将图结构也落盘。
计划采用**独立索引文件**的设计：

* 向量存储：`data.mvdb` (MVDB v1 协议)
* 图索引文件：`data.hnsw` (专属协议，仅保存 `adj_` 邻接表等图拓扑结构)

**冷启动链路规划**：

1. `Serializer::load("data.mvdb")` 极速加载底层向量数据。
2. 检查是否存在同名的 `.hnsw` 索引文件。
3. 如果存在且内部 Checksum 与 `VectorStore` 的数量级匹配，则执行图索引 Load。
4. 如果不存在或已失效，则触发 `HnswIndex::build(store)` 重新构建拓扑图。
