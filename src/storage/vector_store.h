#pragma once
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace minivecdb {
namespace storage {
// VectorStore 负责统一管理向量数据与 ID 映射。
// 设计要点：
// 1) 向量连续存储在 data_（按行铺平），便于遍历和序列化。
// 2) 业务 ID（uint64_t）与内部下标（uint32_t）分离，便于高效检索。
class VectorStore {
 public:
  // dim 表示每条向量的维度，构造后在该实例内固定不变。
  explicit VectorStore(size_t dim) : dim_(dim) {}

  // 预分配 n 条向量的容量，减少后续扩容次数。
  void reserve(size_t n);

  // 新增一条向量，返回内部下标 internal_idx。
  // id 是业务侧主键；dim 必须与构造时的 dim_ 一致。
  uint32_t add(uint64_t id, const float* vec, size_t dim);

  // 根据内部下标获取向量起始指针（长度为 dim_）。
  const float* get_vector(uint32_t internal_idx) const;

  // 根据内部下标获取业务 ID。
  uint64_t get_id(uint32_t internal_idx) const;

  // 当前向量条数。
  size_t size() const { return ids_.size(); }
  // 向量维度。
  size_t dim() const { return dim_; }

 private:
  // 每条向量维度。
  size_t dim_;
  // 扁平化向量数据，布局为 [v0..., v1..., v2...]
  std::vector<float> data_;
  // internal_idx -> id
  std::vector<uint64_t> ids_;
  // id -> internal_idx
  std::unordered_map<uint64_t, uint32_t> id_to_idx_;
};
}  // namespace storage
}  // namespace minivecdb
