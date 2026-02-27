#include <gtest/gtest.h>
#include <vector>
#include "src/storage/vector_store.h"

using namespace minivecdb::storage;

TEST(VectorStoreTest, AddAndGetWorks) {
  VectorStore store(3);  // dim = 3
  std::vector<float> v1 = {1.0, 2.0, 3.0};
  std::vector<float> v2 = {4.0, 5.0, 6.0};

  uint32_t idx1 = store.add(100, v1.data(), v1.size());
  uint32_t idx2 = store.add(200, v2.data(), v2.size());

  EXPECT_EQ(store.size(), 2);
  EXPECT_EQ(idx1, 0);
  EXPECT_EQ(idx2, 1);

  // 验证零拷贝指针内容
  const float* ptr2 = store.get_vector(idx2);
  EXPECT_FLOAT_EQ(ptr2[0], 4.0);
  EXPECT_FLOAT_EQ(ptr2[1], 5.0);
  EXPECT_FLOAT_EQ(ptr2[2], 6.0);
}

TEST(VectorStoreTest, RejectDuplicateId) {
  VectorStore store(2);
  std::vector<float> v = {1.0, 2.0};
  store.add(10, v.data(), v.size());

  EXPECT_THROW(store.add(10, v.data(), v.size()), std::runtime_error);
}

TEST(VectorStoreTest, RejectDimMismatch) {
  VectorStore store(4);                          // store expects 4
  std::vector<float> v_wrong = {1.0, 2.0, 3.0};  // input is 3

  EXPECT_THROW(store.add(1, v_wrong.data(), v_wrong.size()), std::invalid_argument);
}

TEST(VectorStoreTest, IdMappingCorrect) {
  VectorStore store(2);
  std::vector<float> v = {0.1, 0.2};
  uint32_t internal_idx = store.add(999, v.data(), v.size());

  EXPECT_EQ(store.get_id(internal_idx), 999);
}