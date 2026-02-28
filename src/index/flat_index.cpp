#include "src/index/flat_index.h"
#include <algorithm>
#include <queue>
#include <stdexcept>
#include "src/metric/distance.h"

namespace minivecdb {
namespace index {
void FlatIndex::build(const storage::VectorStore& store) {
  // Flat 索引不做预计算，只持有 VectorStore 只读引用。
  store_ = &store;
}
std::vector<Neighbor> FlatIndex::search(const float* query, size_t dim, size_t topk) const {
  if (!store_) {
    throw std::runtime_error("Index not built: VectorStore is null");
  }
  if (!store_->dim() || dim != store_->dim()) {
    throw std::invalid_argument("Dimension mismatch between query and store");
  }
  if (topk == 0)
    return {};

  // 用最大堆维护当前 topK（堆顶是“最差候选”）。
  std::priority_queue<Neighbor> pq;
  size_t total_elements = store_->size();
  for (uint32_t i = 0; i < total_elements; ++i) {
    const float* vec = store_->get_vector(i);
    float dist = metric::l2_distance(query, vec, dim);

    pq.push({i, dist});
    // 超过 K 时弹出当前最差元素，最终保留 K 个最近邻。
    if (pq.size() > topk) {
      pq.pop();
    }
  }

  // 堆内顺序是从差到好，转成升序结果（由近到远）。
  std::vector<Neighbor> res;
  res.reserve(pq.size());
  while (!pq.empty()) {
    res.push_back(pq.top());
    pq.pop();
  }

  std::reverse(res.begin(), res.end());
  return res;
}
}  // namespace index
}  // namespace minivecdb
