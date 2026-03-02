#include "src/index/graph_index.h"
#include <algorithm>
#include <iostream>
#include <queue>
#include <stdexcept>
#include <vector>
#include "src/metric/distance.h"
/*
图算法在build过程中完成了部分运算并存储，search时候节省了大量运算
*/

namespace minivecdb {
namespace index {
struct GreaterNeighbor {
  bool operator()(const Neighbor& a, const Neighbor& b) const {
    if (a.distance == b.distance) {
      return a.internal_idx > b.internal_idx;
    }
    return a.distance > b.distance;
  }
};
void GraphIndex::build(const storage::VectorStore& store) {
  store_ = &store;
  size_t n = store_->size();
  if (n == 0)
    return;
  if (ef_search_ == 0) {
    std::cerr << "Can't build GraphIndex because ef_search_ == 0.\n";
    return;
  }

  adj_.clear();
  adj_.resize(n);

  for (uint32_t i = 1; i < n; ++i) {
    const float* query = store_->get_vector(i);
    size_t search_pool_size = std::max(M_, ef_search_);  // 视野大小，暂定
    auto neighbors = search_layer(query, search_pool_size, 0);

    size_t edges_to_add = std::min(M_, neighbors.size());  // 邻居数
    for (size_t j = 0; j < edges_to_add; ++j) {
      uint32_t neighbor_idx = neighbors[j].internal_idx;
      adj_[i].push_back(neighbor_idx);
      adj_[neighbor_idx].push_back(i);
    }
  }
}

std::vector<Neighbor> GraphIndex::search_layer(const float* query, size_t ef,
                                               uint32_t enter_point) const {
  // 先遣队
  std::priority_queue<Neighbor, std::vector<Neighbor>, GreaterNeighbor> candidates;
  // 背包
  std::priority_queue<Neighbor> top_results;
  // 访问标记
  std::vector<bool> visited(store_->size(), false);
  float enter_dist = metric::l2_distance(query, store_->get_vector(enter_point), store_->dim());
  candidates.push({enter_point, enter_dist});
  top_results.push({enter_point, enter_dist});
  visited[enter_point] = true;

  while (!candidates.empty()) {
    Neighbor curr = candidates.top();
    candidates.pop();
    // candidate是用来找邻居节点的，在被加入candidate同时已经处理完了
    if (curr.distance > top_results.top().distance && top_results.size() >= ef) {
      break;
    }
    for (uint32_t neighbor_idx : adj_[curr.internal_idx]) {
      if (!visited[neighbor_idx]) {
        visited[neighbor_idx] = true;
        float dist = metric::l2_distance(query, store_->get_vector(neighbor_idx), store_->dim());

        if (top_results.size() < ef || top_results.top().distance > dist) {
          candidates.push({neighbor_idx, dist});
          top_results.push({neighbor_idx, dist});
          if (top_results.size() > ef) {
            top_results.pop();
          }
        }
      }
    }
  }
  std::vector<Neighbor> results;
  results.reserve(top_results.size());
  while (!top_results.empty()) {
    results.push_back(top_results.top());
    top_results.pop();
  }
  std::reverse(results.begin(), results.end());
  return results;
}

std::vector<Neighbor> GraphIndex::search(const float* query, size_t dim, size_t topk) const {
  if (!store_) {
    throw std::runtime_error("Index not built: VectorStore is null");
  }
  if (dim != store_->dim()) {
    throw std::invalid_argument("Dimension mismatch between query and store");
  }
  if (store_->size() == 0 || topk == 0)
    return {};
  size_t actual_ef = std::max(topk, ef_search_);

  auto results = search_layer(query, actual_ef, 0);

  if (results.size() > topk) {
    results.resize(topk);
  }
  return results;
}
}  // namespace index
}  // namespace minivecdb
