#include <gtest/gtest.h>
#include <vector>
#include "src/index/flat_index.h"
#include "src/index/hnsw_index.h"
#include "src/storage/vector_store.h"

using namespace minivecdb::storage;
using namespace minivecdb::index;

class HnswIndexTest : public ::testing::Test {
 protected:
  void SetUp() override {
    store = std::make_unique<VectorStore>(2);
    std::vector<std::vector<float>> vecs = {{0.0f, 0.0f}, {1.0f, 1.0f}, {2.0f, 2.0f},
                                            {3.0f, 3.0f}, {4.0f, 4.0f}, {5.0f, 5.0f}};
    for (size_t i = 0; i < vecs.size(); ++i) {
      store->add(100 + i, vecs[i].data(), 2);
    }
  }
  std::unique_ptr<VectorStore> store;
};

// 用例 1：小数据集 + ef_search 很大时，与 Flat 强一致
TEST_F(HnswIndexTest, TinyDatasetMatchesFlatWhenEfHigh) {
  FlatIndex flat_index;
  flat_index.build(*store);

  // ef_search=50 远大于数据集大小 6，必然能全遍历
  HnswIndex hnsw_index(16, 50, 50, 42);
  hnsw_index.build(*store);

  std::vector<float> query = {2.1f, 2.1f};
  auto flat_res = flat_index.search(query.data(), 2, 3);
  auto hnsw_res = hnsw_index.search(query.data(), 2, 3);

  ASSERT_EQ(flat_res.size(), 3);
  ASSERT_EQ(hnsw_res.size(), 3);
  for (size_t i = 0; i < 3; ++i) {
    EXPECT_EQ(flat_res[i].internal_idx, hnsw_res[i].internal_idx);
  }
}

// 用例 2：维度不匹配拒绝
TEST_F(HnswIndexTest, DimMismatchRejected) {
  HnswIndex hnsw_index;
  hnsw_index.build(*store);
  std::vector<float> query = {1.0f, 1.0f, 1.0f};  // 3维
  EXPECT_THROW(hnsw_index.search(query.data(), 3, 1), std::invalid_argument);
}

// 用例 3：未 Build 直接 Search 拒绝
TEST_F(HnswIndexTest, SearchBeforeBuildRejected) {
  HnswIndex hnsw_index;
  std::vector<float> query = {1.0f, 1.0f};
  EXPECT_THROW(hnsw_index.search(query.data(), 2, 1), std::runtime_error);
}

// 用例 4：固定 Seed 保证结果 100% 可复现
TEST_F(HnswIndexTest, DeterministicWithFixedSeed) {
  HnswIndex hnsw1(16, 50, 50, 42);  // Seed = 42
  hnsw1.build(*store);

  HnswIndex hnsw2(16, 50, 50, 42);  // Seed = 42
  hnsw2.build(*store);

  std::vector<float> query = {4.5f, 4.5f};
  auto res1 = hnsw1.search(query.data(), 2, 1);
  auto res2 = hnsw2.search(query.data(), 2, 1);

  ASSERT_FALSE(res1.empty());
  ASSERT_FALSE(res2.empty());
  // 两次构建出的图结构必定完全一样，搜索出的 Top1 距离必须严丝合缝
  EXPECT_EQ(res1[0].internal_idx, res2[0].internal_idx);
  EXPECT_FLOAT_EQ(res1[0].distance, res2[0].distance);
}