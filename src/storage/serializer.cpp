#include "src/storage/serializer.h"
#include <cstdint>
#include <fstream>
#include <stdexcept>

namespace minivecdb {
namespace storage {
// 当前实现只支持 little-endian 平台，保证文件按既定字节序解释。
inline bool is_little_endian() {
  uint32_t num = 1;
  return (*reinterpret_cast<uint8_t*>(&num) == 1);
}
void Serializer::save(const std::string& path, const VectorStore& store) {
  if (!is_little_endian()) {
    throw std::runtime_error("Only little-endian architechtures are supported currently.");
  }
  std::ofstream out(path, std::ios::binary);
  if (!out.is_open()) {
    throw std::runtime_error("Failed to open file for writing: " + path);
  }
  FileHeader header;
  std::memcpy(header.magic, MAGIC_WORD, 4);
  header.version = CURRENT_VERSION;
  header.dim = store.dim();
  header.count = store.size();
  header.reserved1 = 0;
  header.reserved2 = 0;

  // 按文件协议顺序逐字段写入 header，保持可读性与格式显式性。
  out.write(MAGIC_WORD, 4);
  uint32_t version = CURRENT_VERSION;
  out.write(reinterpret_cast<const char*>(&version), sizeof(version));

  uint32_t dim = store.dim();
  out.write(reinterpret_cast<const char*>(&dim), sizeof(dim));

  uint32_t reserved1 = 0;
  out.write(reinterpret_cast<const char*>(&reserved1), sizeof(reserved1));

  uint64_t count = store.size();
  out.write(reinterpret_cast<const char*>(&count), sizeof(count));

  uint64_t reserved2 = 0;
  out.write(reinterpret_cast<const char*>(&reserved2), sizeof(reserved2));
  size_t total_floats = static_cast<size_t>(header.count) * static_cast<size_t>(header.dim);
  out.write(reinterpret_cast<const char*>(store.get_ids_ptr()), header.count * sizeof(uint64_t));
  out.write(reinterpret_cast<const char*>(store.get_data_ptr()), total_floats * sizeof(float));

  if (!out.good()) {
    throw std::runtime_error("Error occuerred while writing to file: " + path);
  }
}

VectorStore Serializer::load(const std::string& path) {
  if (!is_little_endian()) {
    throw std::runtime_error("Only little-endian architectures are supported currently.");
  }

  static_assert(sizeof(float) == 4, "Float must be 32-bit IEEE 754");

  std::ifstream in(path, std::ios::binary);
  if (!in.is_open()) {
    throw std::runtime_error("Failed to open file for reading: " + path);
  }

  FileHeader header;
  // 按 save() 对应顺序读取 header 各字段。
  in.read(header.magic, 4);
  if (std::memcmp(header.magic, MAGIC_WORD, 4) != 0) {
    throw std::runtime_error("Invalid magic word. Not an MVDB file.");
  }
  in.read(reinterpret_cast<char*>(&header.version), sizeof(header.version));
  if (header.version != CURRENT_VERSION) {
    throw std::runtime_error("Unsupported MVDB version: " + std::to_string(header.version));
  }
  in.read(reinterpret_cast<char*>(&header.dim), sizeof(header.dim));
  in.read(reinterpret_cast<char*>(&header.reserved1), sizeof(header.reserved1));
  in.read(reinterpret_cast<char*>(&header.count), sizeof(header.count));
  in.read(reinterpret_cast<char*>(&header.reserved2), sizeof(header.reserved2));
  // 预校验 payload 大小，提前识别截断文件。
  auto current_pos = in.tellg();
  in.seekg(0, std::ios::end);
  auto file_size = in.tellg();
  in.seekg(current_pos, std::ios::beg);
  size_t expected_payload_size =
      (header.count * sizeof(uint64_t)) + (header.count * header.dim * sizeof(float));
  if (file_size - current_pos < expected_payload_size) {
    throw std::runtime_error("File truncated: payload size smaller than header declaration.");
  }

  // 临时缓冲整块读取，再逐条写回 VectorStore，复用其校验逻辑。
  VectorStore store(header.dim);
  store.reserve(header.count);
  std::vector<uint64_t> temp_ids(header.count);
  if (!in.read(reinterpret_cast<char*>(temp_ids.data()), header.count * sizeof(uint64_t))) {
    throw std::runtime_error("Truncated file: failed to read IDs.");
  }
  std::vector<float> temp_vectors(header.count * header.dim);
  if (!in.read(reinterpret_cast<char*>(temp_vectors.data()),
               header.count * header.dim * sizeof(float))) {
    throw std::runtime_error("Truncated file: failed to read vector data.");
  }
  for (size_t i = 0; i < header.count; ++i) {
    store.add(temp_ids[i], temp_vectors.data() + (i * header.dim), header.dim);
  }
  return store;
}
}  // namespace storage
}  // namespace minivecdb
