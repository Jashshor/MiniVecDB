#include "src/storage/vector_store.h"
#include <limits>

namespace minivecdb {
namespace storage {
void VectorStore::reserve(size_t n) {
  // 预留连续内存，避免批量插入时频繁扩容。
  data_.reserve(n * dim_);
  ids_.reserve(n);
  id_to_idx_.reserve(n);
}
uint32_t VectorStore::add(uint64_t id, const float* vec, size_t dim) {
  if (vec == nullptr && dim != 0) {
    throw std::invalid_argument("Input vector pointer is null");
  }
  if (dim != dim_) {
    throw std::invalid_argument("Dimension mismatch");
  }
  if (id_to_idx_.find(id) != id_to_idx_.end()) {
    throw std::runtime_error("Duplicate ID");
  }
  // 当前内部索引类型是 uint32_t，先做上限保护。
  if (ids_.size() > static_cast<size_t>(std::numeric_limits<uint32_t>::max())) {
    throw std::overflow_error("VectorStore internal index exceeds uint32_t range");
  }
  uint32_t internal_idx = static_cast<uint32_t>(ids_.size());
  // 向量按行追加到扁平数组中：[v0..., v1..., v2...]
  data_.insert(data_.end(), vec, vec + dim);
  ids_.push_back(id);
  id_to_idx_[id] = internal_idx;
  return internal_idx;
}
const float* VectorStore::get_vector(uint32_t internal_idx) const {
  if (internal_idx >= ids_.size()) {
    throw std::out_of_range("Internal index out of range.");
  }
  // 第 internal_idx 条向量的起始位置在 internal_idx * dim_。
  return data_.data() + (internal_idx * dim_);
}
uint64_t VectorStore::get_id(uint32_t internal_idx) const {
  if (internal_idx >= ids_.size()) {
    throw std::out_of_range("Internal index out of range.");
  }
  return ids_[internal_idx];
}
}  // namespace storage
}  // namespace minivecdb
