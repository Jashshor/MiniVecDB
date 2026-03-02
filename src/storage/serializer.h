#pragma once
#include <cstdint>
#include <cstring>
#include <string>
#include "src/storage/vector_store.h"

namespace minivecdb {
namespace storage {
#pragma pack(push, 1)
// MVDB 文件头（固定 32 字节）。
// 说明：
// - magic/version 用于文件类型与兼容性校验
// - dim/count 描述 payload 形状
// - reserved 字段为未来扩展预留
struct FileHeader {
  char magic[4];
  uint32_t version;
  uint32_t dim;
  uint32_t reserved1;
  uint64_t count;
  uint64_t reserved2;
};
#pragma pack(pop)
constexpr uint32_t CURRENT_VERSION = 1;
constexpr char MAGIC_WORD[4] = {'M', 'V', 'D', 'B'};

class Serializer {
 public:
  // 将 VectorStore 序列化到磁盘文件。
  static void save(const std::string& path, const VectorStore& store);
  // 从磁盘文件加载并重建 VectorStore。
  static VectorStore load(const std::string& path);
};
}  // namespace storage
}  // namespace minivecdb
