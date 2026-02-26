#include <gtest/gtest.h>
#include <vector>
#include "src/metric/distance.h"

using namespace minivecdb::metric;

TEST(DistanceTest, L2DistanceBasic) {
    std::vector<float> v1 = {1.0f, 2.0f, 3.0f};
    std::vector<float> v2 = {4.0f, 5.0f, 6.0f};
    float dist = l2_distance(v1.data(), v2.data(), v1.size());

    EXPECT_FLOAT_EQ(dist, 27.0f);
}
// TEST(DistanceTest, L2DistanceDimensionMismatch) {
//     std::vector<float> v1 = {1.0f, 2.0f};
//     std::vector<float> v2 = {1.0f, 2.0f, 3.0f};

//     EXPECT_THROW(l2_distance(v1.data(), v2.data()));
// }
