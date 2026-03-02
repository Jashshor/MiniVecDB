#pragma once
#include <vector>
#include "src/index/iindex.h"

namespace minivecdb {
namespace index {
// GraphIndex: 单层图索引（NSW 思路的简化版）。
// 通过图邻居扩展来减少全量扫描，属于近似检索（ANN）。
class GraphIndex : public IIndex {
 public:
  // M: 每个节点的最大边数；ef_search: 查询阶段候选池大小。
  GraphIndex(size_t M = 16, size_t ef_search = 50) : M_(M), ef_search_(ef_search) {}
  // 基于 VectorStore 构建邻接表图结构。
  void build(const storage::VectorStore& store) override;
  // 在图上执行近邻搜索，返回按距离升序的 topK 结果。
  std::vector<Neighbor> search(const float* query, size_t dim, size_t topk) const override;

 private:
  // 只读引用外部向量数据，避免额外内存拷贝。
  const storage::VectorStore* store_ = nullptr;
  // 每个节点最多保留多少条边。
  size_t M_;
  // 查询时探索规模，越大召回通常越高、耗时也更高。
  size_t ef_search_;
  // 邻接表：adj_[u] 存储节点 u 的邻居 internal_idx。
  std::vector<std::vector<uint32_t>> adj_;
  // 图搜索内部子过程：从 enter_point 出发扩展候选，返回前 ef 个候选。
  std::vector<Neighbor> search_layer(const float* query, size_t ef, uint32_t enter_point) const;
};
}  // namespace index
}  // namespace minivecdb
