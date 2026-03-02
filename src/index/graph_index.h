#pragma once
#include <vector>
#include "src/index/iindex.h"

namespace minivecdb {
namespace index {
class GraphIndex : public IIndex {
 public:
  // M: 每个节点的最大边数, ef_search: 搜索时的候选集大小
  GraphIndex(size_t M = 16, size_t ef_search = 50) : M_(M), ef_search_(ef_search) {}
  void build(const storage::VectorStore& store) override;
  std::vector<Neighbor> search(const float* query, size_t dim, size_t topk) const override;

 private:
  // 只引用外部数据，不拷贝整个 store
  const storage::VectorStore* store_ = nullptr;
  size_t M_;
  size_t ef_search_;
  // 邻接表
  std::vector<std::vector<uint32_t>> adj_;
  std::vector<Neighbor> search_layer(const float* query, size_t ef, uint32_t enter_point) const;
};
}  // namespace index
}  // namespace minivecdb