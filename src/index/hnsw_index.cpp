#include "src/index/hnsw_index.h"
#include <sys/types.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <queue>
#include <stdexcept>
#include "src/index/iindex.h"
#include "src/metric/distance.h"

namespace minivecdb {
namespace index {
struct GreaterNeighbor {
  bool operator()(const Neighbor& a, const Neighbor& b) const {
    if (a.distance == b.distance)
      return a.internal_idx > b.internal_idx;
    return a.distance > b.distance;
  }
};

HnswIndex::HnswIndex(size_t M, size_t ef_construction, size_t ef_search, uint32_t seed)
    : M_(M),
      ef_construction_(ef_construction),
      ef_search_(ef_search),
      level_generator_(seed),
      max_level_(-1),
      entry_point_(0),
      current_tag_(0) {
  mult_ = 1.0 / std::log(1.0 * M_);
}

void HnswIndex::build(const storage::VectorStore& store) {
  // dummy implementation
  store_ = &store;
  size_t n = store_->size();
  if (n == 0)
    return;

  nodes_.clear();    //!
  nodes_.resize(n);  //!
  visited_tag_.assign(n, 0);
  current_tag_ = 0;

  max_level_ = random_level();
  entry_point_ = 0;
  nodes_[0].level = max_level_;
  nodes_[0].neighbors.resize(max_level_ + 1);  //!

  for (uint32_t i = 1; i < n; ++i) {
    int l_new = random_level();
    nodes_[i].level = l_new;
    nodes_[i].neighbors.resize(l_new + 1);

    const float* query = store_->get_vector(i);
    uint32_t ep = entry_point_;

    for (int l = max_level_; l > l_new; --l) {
      auto res = search_layer(query, ep, 1, l);
      if (!res.empty()) {
        ep = res[0].internal_idx;
      } else {
        std::cerr << "Failed when building HnswIndex.";
      }
    }
    for (int l = std::min(max_level_, l_new); l >= 0; --l) {
      auto candidates = search_layer(query, ep, ef_construction_, l);
      auto selected_neighbors = SelectNeighbors(candidates, M_);

      for (uint32_t neighbor_idx : selected_neighbors) {
        nodes_[i].neighbors[l].push_back(neighbor_idx);
        nodes_[neighbor_idx].neighbors[l].push_back(i);
      }
      if (!candidates.empty()) {
        ep = candidates[0].internal_idx;
      }
    }
    if (l_new > max_level_) {
      max_level_ = l_new;
      entry_point_ = i;
    }
  }
}

std::vector<Neighbor> HnswIndex::search(const float* query, size_t dim, size_t topk) const {
  if (!store_)
    throw std::runtime_error("Index not built");
  if (dim != store_->dim())
    throw std::invalid_argument("Dimension mismatch");
  if (store_->size() == 0)
    return {};

  uint32_t ep = entry_point_;
  for (int l = max_level_; l >= 1; --l) {
    auto res = search_layer(query, ep, 1, l);
    if (!res.empty()) {
      ep = res[0].internal_idx;
    }
  }

  size_t ef = std::max(topk, ef_search_);
  auto res = search_layer(query, ep, ef, 0);
  if (res.size() > topk) {
    res.resize(topk);
  }
  return res;
}

int HnswIndex::random_level() {
  std::uniform_real_distribution<double> distribution(0.0, 1.0);
  double r = -log(distribution(level_generator_)) * mult_;
  return (int)r;
}

std::vector<Neighbor> HnswIndex::search_layer(const float* query, uint32_t ep, size_t ef,
                                              int layer) const {
  ++current_tag_;
  std::priority_queue<Neighbor, std::vector<Neighbor>, GreaterNeighbor> candidates;
  std::priority_queue<Neighbor> top_results;

  float ep_dist = metric::l2_distance(query, store_->get_vector(ep), store_->dim());
  candidates.push({ep, ep_dist});
  top_results.push({ep, ep_dist});
  visited_tag_[ep] = current_tag_;

  while (!candidates.empty()) {
    Neighbor curr = candidates.top();
    candidates.pop();
    if (curr.distance > top_results.top().distance && top_results.size() >= ef) {
      break;
    }
    for (uint32_t neighbor_idx : nodes_[curr.internal_idx].neighbors[layer]) {
      if (visited_tag_[neighbor_idx] != current_tag_) {
        visited_tag_[neighbor_idx] = current_tag_;
        float dist = metric::l2_distance(query, store_->get_vector(neighbor_idx), store_->dim());
        if (top_results.size() < ef || dist < top_results.top().distance) {
          top_results.push({neighbor_idx, dist});
          candidates.push({neighbor_idx, dist});
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

std::vector<uint32_t> HnswIndex::SelectNeighbors(const std::vector<Neighbor>& candidates,
                                                 size_t M) const {
  std::vector<uint32_t> res;
  size_t limit = std::min(M, candidates.size());
  for (size_t i = 0; i < limit; ++i) {
    res.push_back(candidates[i].internal_idx);
  }
  return res;
}

}  // namespace index
}  // namespace minivecdb
