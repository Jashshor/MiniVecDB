#pragma once
#include <cstdint>
#include <utility>
#include <vector>

namespace minivecdb {
namespace index {
class IIndex {
 public:
  virtual ~IIndex() = default;
  virtual void build() = 0;
  virtual std::vector<std::pair<uint64_t, float>> search(const float* query, size_t topK) const = 0;
};
}  // namespace index
}  // namespace minivecdb