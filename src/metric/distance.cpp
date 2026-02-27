#include "src/metric/distance.h"
#include <stdexcept>

namespace minivecdb {
namespace metric {
float l2_distance(const float* vec1, const float* vec2, size_t dim) {
    // 这里返回的是 L2 的平方和（未开根号），用于排序通常已足够。
    float distance = 0.0f;
    for (size_t i = 0; i < dim; ++i) {
        float diff = vec1[i] - vec2[i];
        distance += diff * diff;
    }
    return distance;
}
}
}
