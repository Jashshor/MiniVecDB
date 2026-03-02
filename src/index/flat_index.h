#pragma once
#include "src/index/iindex.h"

namespace minivecdb {
namespace index {
// FlatIndex: 暴力检索基线实现。
// 特点：实现简单、结果精确；代价是查询复杂度 O(N * dim)。
class FlatIndex : public IIndex {
 public:
  FlatIndex() = default;

  // Flat 索引无需预构建图结构，仅保存对 VectorStore 的只读引用。
  void build(const storage::VectorStore& store) override;

  // 返回按距离从近到远排序的 topK 结果（internal_idx + distance）。
  std::vector<Neighbor> search(const float* query, size_t dim, size_t topK) const override;

 private:
  // 生命周期由外部管理；调用方需保证 build 后 store 持续有效。
  const storage::VectorStore* store_ = nullptr;
};
}  // namespace index
}  // namespace minivecdb
