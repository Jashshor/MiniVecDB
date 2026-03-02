#include <gtest/gtest.h>
#include <vector>
#include "src/index/flat_index.h"
#include "src/index/graph_index.h"
#include "src/storage/vector_store.h"

using namespace minivecdb::storage;
using namespace minivecdb::index;

class GraphIndexTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // 构建一个极小的数据集 (N=5, Dim=2)
    store = std::make_unique<VectorStore>(2);

    std::vector<float> v0 = {0.0f, 0.0f};
    std::vector<float> v1 = {1.0f, 1.0f};
    std::vector<float> v2 = {2.0f, 2.0f};
    std::vector<float> v3 = {3.0f, 3.0f};
    std::vector<float> v4 = {4.0f, 4.0f};

    store->add(100, v0.data(), 2);  // idx 0
    store->add(101, v1.data(), 2);  // idx 1
    store->add(102, v2.data(), 2);  // idx 2
    store->add(103, v3.data(), 2);  // idx 3
    store->add(104, v4.data(), 2);  // idx 4
  }

  std::unique_ptr<VectorStore> store;
};

// 用例 1：小图精确匹配测试 (Cross Validation with FlatIndex)
TEST_F(GraphIndexTest, SmallGraphExactMatch) {
  // 建立绝对真值裁判
  FlatIndex flat_index;
  flat_index.build(*store);

  // 建立图索引。因为我们设置 M=16，而数据总量才 5 条 (N < M)，
  // 所以这张图在构建完毕后，必然是一个“全连接图” (Fully Connected Graph)。
  // 在全连接图上做贪心搜索，结果必须和暴力全量扫描 100% 一致！
  GraphIndex graph_index(16, 50);
  graph_index.build(*store);

  std::vector<float> query = {2.1f, 2.1f};  // 目标靠近 v2

  auto flat_res = flat_index.search(query.data(), 2, 3);
  auto graph_res = graph_index.search(query.data(), 2, 3);

  ASSERT_EQ(flat_res.size(), 3);
  ASSERT_EQ(graph_res.size(), 3);

  // 逐个比对 TopK 结果，内部 ID 和距离必须严丝合缝
  for (size_t i = 0; i < 3; ++i) {
    EXPECT_EQ(flat_res[i].internal_idx, graph_res[i].internal_idx);
    EXPECT_FLOAT_EQ(flat_res[i].distance, graph_res[i].distance);
  }
}

// 用例 2：未构建索引的异常拦截
TEST_F(GraphIndexTest, UnbuiltIndexThrows) {
  GraphIndex graph_index;
  std::vector<float> query = {1.0f, 1.0f};

  EXPECT_THROW(graph_index.search(query.data(), 2, 1), std::runtime_error);
}

// 用例 3：查询维度不匹配的异常拦截
TEST_F(GraphIndexTest, DimMismatchRejected) {
  GraphIndex graph_index;
  graph_index.build(*store);

  std::vector<float> query = {1.0f, 1.0f, 1.0f};  // 故意传入 3 维数据
  EXPECT_THROW(graph_index.search(query.data(), 3, 1), std::invalid_argument);
}