#pragma once
#include <cstddef>

namespace minivecdb {
namespace metric {
// 计算 L2 距离的平方和（不做 sqrt）。
// 该形式常用于向量检索排序，避免开方带来的额外开销。
float l2_distance(const float* vec1, const float* vec2, size_t dim);
}
}
