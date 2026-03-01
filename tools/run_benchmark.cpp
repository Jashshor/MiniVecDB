#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
#include "src/index/flat_index.h"
#include "src/storage/serializer.h"
#include "src/utils/timer.h"

using namespace minivecdb;

int main(int argc, char** argv) {
  if (argc != 4) {
    std::cerr << "Usage: " << argv[0] << " <data.mvdb> <num_queries> <topk>\n";
    return 1;
  }
  std::string file_path = argv[1];
  size_t q_count = std::stoull(argv[2]);
  size_t topk = std::stoull(argv[3]);

  // 核心改动：用工业级方案瞬间 Load
  storage::VectorStore store = storage::Serializer::load(file_path);
  size_t n = store.size();
  size_t dim = store.dim();

  index::FlatIndex flat_index;
  flat_index.build(store);

  std::cout << "Running Benchmark on " << n << " vectors (" << q_count << " queries, TopK=" << topk
            << ")...\n";
  std::vector<double> latencies;
  latencies.reserve(q_count);

  utils::Timer total_timer;
  for (size_t i = 0; i < q_count; ++i) {
    const float* query = store.get_vector(i % n);  // 循环复用数据作为 query
    utils::Timer q_timer;
    auto res = flat_index.search(query, dim, topk);
    latencies.push_back(q_timer.elapsed_ms());
  }
  double total_time = total_timer.elapsed_ms();

  std::sort(latencies.begin(), latencies.end());
  std::cout << "--- Benchmark Results ---\n";
  std::cout << "Avg Latency: " << (total_time / q_count) << " ms\n";
  std::cout << "P95 Latency: " << latencies[static_cast<size_t>(q_count * 0.95)] << " ms\n";
  std::cout << "QPS        : " << (q_count / (total_time / 1000.0)) << " queries/sec\n";

  return 0;
}
