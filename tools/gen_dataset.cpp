#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <random>
#include <string>
#include <vector>
#include "src/storage/serializer.h"

using namespace minivecdb::storage;

int main(int argc, char** argv) {
  if (argc != 4) {
    std::cerr << "Usage: " << argv[0] << " <N> <dim> <output_file>\n";
    std::cerr << "Example: " << argv[0] << " 10000 128 data.bin\n";
    return 1;
  }
  uint64_t n = std::stoull(argv[1]);
  uint64_t dim = std::stoull(argv[2]);
  if (dim > static_cast<uint64_t>(std::numeric_limits<size_t>::max())) {
    std::cerr << "[ERROR] dim is too large for this platform\n";
    return 1;
  }
  if (n > static_cast<uint64_t>(std::numeric_limits<uint32_t>::max()) ||
      dim > static_cast<uint64_t>(std::numeric_limits<uint32_t>::max())) {
    std::cerr << "[ERROR] n/dim exceeds MVDB v1 header uint32 range\n";
    return 1;
  }
  std::string out_path = argv[3];

  std::ofstream out(out_path, std::ios::binary);
  if (!out.is_open()) {
    std::cerr << "[ERROR] Failed to open: " << out_path << "\n";
    return 1;
  }
  // 1. 写入标准的 32 字节 Header
  FileHeader header;
  std::memcpy(header.magic, MAGIC_WORD, 4);
  header.version = CURRENT_VERSION;
  header.dim = dim;
  header.reserved1 = 0;
  header.count = n;
  header.reserved2 = 0;
  out.write(reinterpret_cast<const char*>(&header), sizeof(FileHeader));

  // 2. 生成并写入真实的业务 ID (这里模拟使用 10000 起始的递增 ID)
  std::vector<uint64_t> ids(n);
  for (uint64_t i = 0; i < n; ++i) {
    ids[i] = 10000 + i;
  }
  out.write(reinterpret_cast<const char*>(ids.data()), n * sizeof(uint64_t));

  // 3. 生成并写入连续的 Float 向量数据
  std::mt19937 gen(42);
  std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
  std::vector<float> vec(n * dim);
  for (size_t i = 0; i < n * dim; ++i) {
    vec[i] = dist(gen);
  }
  out.write(reinterpret_cast<const char*>(vec.data()), n * dim * sizeof(float));

  out.close();
  std::cout << "[SUCCESS] Generated MVDB v1 dataset: " << out_path << "\n";
  return 0;
}
