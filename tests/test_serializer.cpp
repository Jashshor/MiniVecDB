#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include "src/storage/serializer.h"
#include "src/storage/vector_store.h"

using namespace minivecdb::storage;

class SerializerTest : public ::testing::Test {
 protected:
  const std::string test_file = "test_data.mvdb";
  void TearDown() override {
    std::remove(test_file.c_str());  // 跑完清理现场
  }
};

TEST_F(SerializerTest, SaveLoadRoundTripBasic) {
  VectorStore original_store(3);
  std::vector<float> v1 = {1.0, 2.0, 3.0};
  std::vector<float> v2 = {4.0, 5.0, 6.0};

  original_store.add(10, v1.data(), 3);
  original_store.add(20, v2.data(), 3);

  Serializer::save(test_file, original_store);
  VectorStore loaded_store = Serializer::load(test_file);

  // 断言：元数据一致
  EXPECT_EQ(loaded_store.dim(), 3);
  EXPECT_EQ(loaded_store.size(), 2);

  // 断言：ID 和向量内容完全一致
  EXPECT_EQ(loaded_store.get_id(0), 10);
  EXPECT_EQ(loaded_store.get_id(1), 20);

  const float* loaded_v1 = loaded_store.get_vector(0);
  EXPECT_FLOAT_EQ(loaded_v1[0], 1.0);
  EXPECT_FLOAT_EQ(loaded_v1[2], 3.0);
}

TEST_F(SerializerTest, RejectBadMagic) {
  VectorStore store(2);
  Serializer::save(test_file, store);

  // 暴力破坏文件的 Magic Word
  std::fstream file(test_file, std::ios::in | std::ios::out | std::ios::binary);
  file.seekp(0);
  file.write("Hero", 4);
  file.close();

  EXPECT_THROW(Serializer::load(test_file), std::runtime_error);
}

TEST_F(SerializerTest, RejectUnsupportedVersion) {
  VectorStore store(2);
  Serializer::save(test_file, store);

  // 把 Version (第 4 到 7 字节) 改成 999
  uint32_t bad_version = 999;
  std::fstream file(test_file, std::ios::in | std::ios::out | std::ios::binary);
  file.seekp(4);
  file.write(reinterpret_cast<char*>(&bad_version), sizeof(bad_version));
  file.close();

  EXPECT_THROW(Serializer::load(test_file), std::runtime_error);
}

TEST_F(SerializerTest, RejectTruncatedFile) {
  VectorStore store(5);  // dim 5
  std::vector<float> v = {1, 2, 3, 4, 5};
  store.add(1, v.data(), 5);
  Serializer::save(test_file, store);

  // 裁剪文件，截断 Payload 部分
  std::filesystem::resize_file(test_file, 40);  // 假设我们只保留头部的 40 字节

  EXPECT_THROW(Serializer::load(test_file), std::runtime_error);
}
