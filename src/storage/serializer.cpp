#include "src/storage/serializer.h"
#include <fstream>
#include <stdexcept>

namespace minivecdb {
namespace storage {
void Serializer::save(const std::string& path, const VectorStore& store) {
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

  out.write(reinterpret_cast<const char*>(&header), sizeof(FileHeader));
  size_t total_floats = static_cast<size_t>(header.count) * static_cast<size_t>(header.dim);
  out.write(reinterpret_cast<const char*>(store.get_ids_ptr()), header.count * sizeof(uint64_t));
  out.write(reinterpret_cast<const char*>(store.get_data_ptr()), total_floats * sizeof(float));

  if (!out.good()) {
    throw std::runtime_error("Error occuerred while writing to file: " + path);
  }
}

VectorStore Serializer::load(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in.is_open()) {
    throw std::runtime_error("Failed to open file for reading: " + path);
  }

  FileHeader header;
  if (!in.read(reinterpret_cast<char*>(&header), sizeof(FileHeader))) {
    throw std::runtime_error("File is too small or truncated: " + path);
  }
  if (std::memcmp(header.magic, MAGIC_WORD, 4) != 0) {
    throw std::runtime_error("Invalid magic word. Not an MVDB file.");
  }
  if (header.version != CURRENT_VERSION) {
    throw std::runtime_error("Unsupported MVDB version: " + std::to_string(header.version));
  }

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
