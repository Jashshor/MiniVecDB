#pragma once
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>
#include "src/storage/vector_store.h"

namespace minivecdb {
namespace index {
// 邻居候选：内部下标 + 与查询向量的距离。
// internal_idx 指向 VectorStore 内部位置，不是业务 ID。
struct Neighbor {
  uint32_t internal_idx;
  float distance;
  // 用于排序时按距离比较（距离越小越近）。
  bool operator<(const Neighbor& other) const {
    if (distance == other.distance) {
      return internal_idx < other.internal_idx;
    }
    return distance < other.distance;
  }
};

// 索引统一接口：FlatIndex / HnswIndex 都应实现该接口，
// 这样上层 Engine 可以在不改业务代码的前提下切换索引实现。
class IIndex {
 public:
  virtual ~IIndex() = default;

  // 构建索引结构。
  // 对 FlatIndex 可为空实现；对 HNSW 通常会根据 VectorStore 建图。
  virtual void build(const storage::VectorStore& store) = 0;

  // 查询 topK：
  // - query: 查询向量指针，长度应与 collection dim 一致
  // - topK: 返回结果数量上限
  // 返回值：[(id, distance)]，id 是业务 ID，distance 越小表示越相似（以 L2 为例）
  virtual std::vector<Neighbor> search(const float* query, size_t dim, size_t topK) const = 0;
};
}  // namespace index
}  // namespace minivecdb
