#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <numeric>
#include <string>
#include <vector>
#include "src/index/flat_index.h"
#include "src/storage/vector_store.h"
#include "src/utils/timer.h"

using namespace minivecdb;

namespace {
// 与 gen_dataset 保持一致：按 little-endian 读取 uint64 头字段。
bool read_u64_le(std::ifstream& in, uint64_t* value) {
  unsigned char bytes[8];
  if (!in.read(reinterpret_cast<char*>(bytes), sizeof(bytes))) {
    return false;
  }
  uint64_t result = 0;
  for (int i = 0; i < 8; ++i) {
    result |= (static_cast<uint64_t>(bytes[i]) << (i * 8));
  }
  *value = result;
  return true;
}
}  // namespace

int main(int argc, char** argv) {
  if (argc != 4) {
    std::cerr << "Usage: " << argv[0] << " <data.bin> <num_queries> <topk>\n";
    return 1;
  }
  std::string file_path = argv[1];
  size_t q_count = 0;
  size_t topk = 0;
  try {
    q_count = std::stoull(argv[2]);
    topk = std::stoull(argv[3]);
  } catch (const std::exception& e) {
    std::cerr << "[ERROR] Invalid numeric argument: " << e.what() << "\n";
    return 1;
  }
  if (q_count == 0 || topk == 0) {
    std::cerr << "[ERROR] num_queries and topk must be greater than 0\n";
    return 1;
  }

  std::ifstream in(file_path, std::ios::binary);
  if (!in.is_open()) {
    std::cerr << "[ERROR] Failed to open file: " << file_path << "\n";
    return 1;
  }
  uint64_t n64 = 0;
  uint64_t dim64 = 0;
  if (!read_u64_le(in, &n64) || !read_u64_le(in, &dim64)) {
    std::cerr << "[ERROR] Failed to read dataset header\n";
    return 1;
  }
  if (n64 == 0 || dim64 == 0) {
    std::cerr << "[ERROR] Dataset header contains zero n or dim\n";
    return 1;
  }
  if (n64 > static_cast<uint64_t>(std::numeric_limits<size_t>::max()) ||
      dim64 > static_cast<uint64_t>(std::numeric_limits<size_t>::max())) {
    std::cerr << "[ERROR] n/dim is too large for this platform\n";
    return 1;
  }
  const size_t n = static_cast<size_t>(n64);
  const size_t dim = static_cast<size_t>(dim64);

  // 先将数据全部加载进内存，便于稳定评估纯检索性能。
  storage::VectorStore store(dim);
  store.reserve(n);
  std::vector<float> buffer(dim);
  for (size_t i = 0; i < n; ++i) {
    if (!in.read(reinterpret_cast<char*>(buffer.data()), dim * sizeof(float))) {
      std::cerr << "[ERROR] Unexpected EOF while reading vector data at row " << i << "\n";
      return 1;
    }
    store.add(i, buffer.data(), dim);
  }

  index::FlatIndex flat_index;
  flat_index.build(store);

  std::cout << "Running Benchmark with " << q_count << " queries (Topk = " << topk << " ...\n";
  std::vector<double> latencies;
  latencies.reserve(q_count);

  utils::Timer total_timer;

  for (size_t i = 0; i < q_count; ++i) {
    // 用已有向量循环作为查询集合，避免额外 IO 影响 benchmark。
    const float* query = store.get_vector(i % n);
    utils::Timer q_timer;
    auto res = flat_index.search(query, dim, topk);
    (void)res;  // 当前 benchmark 只统计延迟与吞吐。
    latencies.push_back(q_timer.elapsed_ms());
  }
  double total_time = total_timer.elapsed_ms();

  std::sort(latencies.begin(), latencies.end());
  double avg = total_time / q_count;
  size_t p95_index = static_cast<size_t>(q_count * 0.95);
  if (p95_index >= latencies.size()) {
    p95_index = latencies.empty() ? 0 : (latencies.size() - 1);
  }
  double p95 = latencies.empty() ? 0.0 : latencies[p95_index];
  double qps = q_count / (total_time / 1000.0);
  std::cout << "--- Benchmark Results ---\n";
  std::cout << "Total Time : " << total_time << " ms\n";
  std::cout << "Avg Latency: " << avg << " ms\n";
  std::cout << "P95 Latency: " << p95 << " ms\n";
  std::cout << "QPS        : " << qps << " queries/sec\n";

  return 0;
}
