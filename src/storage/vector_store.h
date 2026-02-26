#pragma once
#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace minivecdb {
namespace storage {
class VectorStore {
 public:
  explicit VectorStore(size_t dim) : dim_(dim) {}
  uint32_t add(uint64_t id, const std::vector<float>& vec);
  const float* get_vector(uint32_t internal_idx) const;
  size_t size() const { return ids_.size(); }
  size_t dim() const { return dim_; }

 private:
  size_t dim_;
  std::vector<float> data_;
  std::vector<uint64_t> ids_;
  std::unordered_map<uint64_t, uint32_t> id_to_idx_;
};
}  // namespace storage
}  // namespace minivecdb