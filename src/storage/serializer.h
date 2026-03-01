#pragma once
#include <cstdint>
#include <cstring>
#include <string>
#include "src/storage/vector_store.h"

namespace minivecdb {
namespace storage {
#pragma pack(push, 1)
// 32字节头持久化文件头定义
struct FileHeader {
  char magic[4];
  uint32_t version;
  uint32_t dim;
  uint32_t reserved1;
  uint32_t count;
  uint32_t reserved2;
};
#pragma pack(pop)
constexpr uint32_t CURRENT_VERSION = 1;
constexpr char MAGIC_WORD[4] = {'M', 'V', 'D', 'B'};

class Serializer {
 public:
  static void save(const std::string& path, const VectorStore& store);
  static VectorStore load(const std::string& path);
};
}  // namespace storage
}  // namespace minivecdb