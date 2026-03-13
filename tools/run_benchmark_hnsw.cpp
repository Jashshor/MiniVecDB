#include <algorithm>
#include <iomanip>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>
#include "src/index/flat_index.h"
#include "src/index/hnsw_index.h"
#include "src/storage/serializer.h"
#include "src/utils/timer.h"

using namespace minivecdb;

// 增加一个 use_csv 参数来控制输出格式
void run_single_eval(const storage::VectorStore& store, const index::FlatIndex& flat_index,
                     size_t q_count, size_t topk, size_t M, size_t efC, size_t efS, bool use_csv) {
  size_t n = store.size();
  size_t dim = store.dim();

  utils::Timer build_timer;
  index::HnswIndex hnsw_index(M, efC, efS, 42);  // 固定 seed 保证实验可复现
  hnsw_index.build(store);
  double build_ms = build_timer.elapsed_ms();

  std::vector<double> latencies;
  latencies.reserve(q_count);
  double total_recall = 0.0;
  double total_search_time = 0.0;

  for (size_t i = 0; i < q_count; ++i) {
    const float* query = store.get_vector(i % n);

    auto gt_res = flat_index.search(query, dim, topk);
    std::unordered_set<uint32_t> gt_set;
    for (const auto& neighbor : gt_res) gt_set.insert(neighbor.internal_idx);

    utils::Timer q_timer;
    auto pred_res = hnsw_index.search(query, dim, topk);
    double cost = q_timer.elapsed_ms();
    latencies.push_back(cost);
    total_search_time += cost;

    size_t hit = 0;
    for (const auto& neighbor : pred_res) {
      if (gt_set.count(neighbor.internal_idx))
        hit++;
    }
    total_recall += static_cast<double>(hit) / topk;
  }

  std::sort(latencies.begin(), latencies.end());
  double avg_recall = total_recall / q_count;
  double avg_ms = total_search_time / q_count;
  double p95_ms = latencies[static_cast<size_t>(q_count * 0.95)];
  double qps = q_count / (total_search_time / 1000.0);

  // 根据模式决定输出格式
  if (use_csv) {
    // CSV 格式：扫参模式专用
    std::cout << n << "," << dim << "," << q_count << "," << topk << "," << M << "," << efC << ","
              << efS << "," << std::fixed << std::setprecision(2) << build_ms << "," << avg_ms
              << "," << p95_ms << "," << qps << "," << (avg_recall * 100.0) << "\n";
  } else {
    // 人类可读格式：单次测试专用
    std::cout << "\n=== HnswIndex Benchmark Results ===\n";
    std::cout << "Parameters  : M=" << M << ", efConstruction=" << efC << ", efSearch=" << efS
              << "\n";
    std::cout << "Build Time  : " << build_ms << " ms\n";
    std::cout << "Recall@" << topk << "   : " << (avg_recall * 100.0) << " %\n";
    std::cout << "Avg Latency : " << avg_ms << " ms\n";
    std::cout << "P95 Latency : " << p95_ms << " ms\n";
    std::cout << "QPS         : " << qps << " queries/sec\n";
    std::cout << "===================================\n";
  }
}

int main(int argc, char** argv) {
  if (argc < 3) {
    std::cerr << "Usage: \n"
              << "  Single: " << argv[0] << " <data.mvdb> <Q> <topk> <M> <efC> <efS>\n"
              << "  Sweep : " << argv[0] << " <data.mvdb> --sweep\n";
    return 1;
  }

  std::string file_path = argv[1];
  std::string mode = argv[2];

  std::cerr << "[Info] Loading data from " << file_path << "...\n";
  storage::VectorStore store = storage::Serializer::load(file_path);

  std::cerr << "[Info] Building FlatIndex for Ground Truth...\n";
  index::FlatIndex flat_index;
  flat_index.build(store);

  if (mode == "--sweep") {
    // 扫参模式
    std::cout << "N,dim,Q,topk,M,efC,efS,build_ms,avg_ms,p95_ms,qps,recall@K(%)\n";
    size_t Q = 1000, topk = 10;
    std::vector<std::tuple<size_t, size_t, size_t>> sweep_params = {
        {16, 100, 50}, {16, 200, 100}, {32, 200, 100}, {32, 400, 200}};

    std::cerr << "[Info] Running Sweep Mode...\n";
    for (const auto& params : sweep_params) {
      // 传入 true，使用 CSV
      run_single_eval(store, flat_index, Q, topk, std::get<0>(params), std::get<1>(params),
                      std::get<2>(params), true);
    }
  } else {
    // 单次执行模式
    if (argc != 7) {
      std::cerr << "Error: Missing parameters for single run.\n";
      return 1;
    }
    size_t Q = std::stoull(argv[2]);
    size_t topk = std::stoull(argv[3]);
    size_t M = std::stoull(argv[4]);
    size_t efC = std::stoull(argv[5]);
    size_t efS = std::stoull(argv[6]);

    std::cerr << "[Info] Running Single Evaluation Mode...\n";
    // 传入 false，使用人类可读排版
    run_single_eval(store, flat_index, Q, topk, M, efC, efS, false);
  }

  return 0;
}