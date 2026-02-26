#pragma once
#include <cstddef>

namespace minivecdb {
    namespace metric {
        // L2 distance    
        float l2_distance(const float* vec1, const float* vec2, size_t dim);
    }
}