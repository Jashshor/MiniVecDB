#include <gtest/gtest.h>
#include <memory>
#include <vector>
#include "src/index/flat_index.h"
#include "src/storage/vector_store.h"

using namespace minivecdb::storage;
using namespace minivecdb::index;

class FlatIndexTest : public ::testing::Test {
 protected:
  void SetUp() override {
    store = std::make_unique<VectorStore>(2);
    std::vector<float> v0 = {0.0f, 0.0f};
    std::vector<float> v1 = {1.0f, 1.0f};
    std::vector<float> v2 = {2.0f, 2.0f};

    store->add(100, v0.data(), 2);  // idx 0
    store->add(101, v1.data(), 2);  // idx 1
    store->add(102, v2.data(), 2);  // idx 2

    index.build(*store);
  }
  std::unique_ptr<VectorStore> store;
  FlatIndex index;
};

TEST_F(FlatIndexTest, SmallExactMatch) {
  std::vector<float> query = {1.0f, 1.0f};
  auto res = index.search(query.data(), 2, 1);
  ASSERT_EQ(res.size(), 1u);
  EXPECT_EQ(res[0].internal_idx, 1u);
  EXPECT_FLOAT_EQ(res[0].distance, 0.0f);
}

TEST_F(FlatIndexTest, TopKGreaterThanN) {
  std::vector<float> query = {0.0f, 0.0f};
  auto results = index.search(query.data(), 2, 10);
  EXPECT_EQ(results[0].internal_idx, 0);
  EXPECT_EQ(results[1].internal_idx, 1);
  EXPECT_EQ(results[2].internal_idx, 2);
}

TEST_F(FlatIndexTest, DimMismatchRejected) {
  std::vector<float> query = {1.0f, 1.0f, 1.0f};  // 传了 3 维
  EXPECT_THROW(index.search(query.data(), 3, 1), std::invalid_argument);
}

TEST_F(FlatIndexTest, DeterministicOrderOnTie) {
  VectorStore tie_store(1);
  std::vector<float> v = {1.0f};
  // 插入两个完全相同的向量
  tie_store.add(10, v.data(), 1);  // idx 0
  tie_store.add(11, v.data(), 1);  // idx 1

  FlatIndex tie_index;
  tie_index.build(tie_store);

  std::vector<float> query = {1.0f};
  auto results = tie_index.search(query.data(), 1, 2);

  ASSERT_EQ(results.size(), 2);
  EXPECT_EQ(results[0].internal_idx, 0);  // 内部 idx 小的优先
  EXPECT_EQ(results[1].internal_idx, 1);
}

TEST_F(FlatIndexTest, UnbuiltIndexThrows) {
  FlatIndex empty_index;
  std::vector<float> query = {1.0f};
  EXPECT_THROW(empty_index.search(query.data(), 1, 1), std::runtime_error);
}