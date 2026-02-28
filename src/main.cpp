#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include "src/index/flat_index.h"
#include "src/storage/vector_store.h"
#include "src/utils/logging.h"
#include "src/utils/timer.h"

using namespace minivecdb;

int main(int argc, char** argv) {
  if (argc != 3) {
    std::cerr << "Usage: " << argv[0] << " <data.bin> <topk>\n";
    return 1;
  }
  std::string file_path = argv[1];
  size_t topk = std::stoull(argv[2]);

  LOG_INFO("Loading data from " << file_path);
  std::ifstream in(file_path, std::ios::binary);
  if (!in.is_open()) {
    LOG_ERROR("Failed to open file");
    return 1;
  }

  size_t n = 0, dim = 0;
  // 读取数据头：当前写法按本机 size_t 读取，需与写入格式保持一致。
  in.read(reinterpret_cast<char*>(&n), sizeof(n));
  in.read(reinterpret_cast<char*>(&dim), sizeof(dim));

  LOG_INFO("Dataset info: N = " << n << " , Dim = " << dim);
  storage::VectorStore store(dim);
  store.reserve(n);

  std::vector<float> buffer(dim);
  // 逐行加载向量并写入 VectorStore，内部会按扁平数组连续存储。
  for (size_t i = 0; i < n; ++i) {
    in.read(reinterpret_cast<char*>(buffer.data()), dim * sizeof(float));
    store.add(i, buffer.data(), dim);
  }
  in.close();

  LOG_INFO("Building FlatIndex...");
  index::FlatIndex flat_index;
  flat_index.build(store);

  LOG_INFO("Searching Top " << topk << " ...");
  utils::Timer timer;
  // 这里使用第 0 条向量作为示例查询向量。
  const float* query = store.get_vector(0);
  auto res = flat_index.search(query, dim, topk);
  double latency = timer.elapsed_ms();

  LOG_INFO("Searching completed in " << latency << " ms.");
  for (size_t i = 0; i < res.size(); ++i) {
    std::cout << "Rank " << i + 1 << ": ID " << store.get_id(res[i].internal_idx)
              << ", Distance = " << res[i].distance << std::endl;
  }

  return 0;
}
