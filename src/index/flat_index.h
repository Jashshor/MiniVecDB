#pragma once
#include "src/index/iindex.h"

namespace minivecdb {
namespace index {
class FlatIndex : public IIndex {
 public:
  FlatIndex() = default;
  void build(const storage::VectorStore& store) override;
  std::vector<Neighbor> search(const float* query, size_t dim, size_t topK) const override;

 private:
  const storage::VectorStore* store_ = nullptr;
};
}  // namespace index
}  // namespace minivecdb