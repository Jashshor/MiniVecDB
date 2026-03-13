#include <algorithm>
#include <iostream>
#include <unordered_set>
#include <vector>
#include "src/index/flat_index.h"
#include "src/index/graph_index.h"
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

  // 1. 极速加载持久化数据
  std::cout << "[1/4] Loading data from " << file_path << "...\n";
  storage::VectorStore store = storage::Serializer::load(file_path);
  size_t n = store.size();
  size_t dim = store.dim();

  // 2. 构建 FlatIndex (绝对真值基线)
  std::cout << "[2/4] Building FlatIndex (Ground Truth)...\n";
  index::FlatIndex flat_index;
  flat_index.build(store);

  // 3. 构建 GraphIndex (单层 NSW 测试对象)
  std::cout << "[3/4] Building GraphIndex (M=16, ef_search=50)...\n";
  utils::Timer build_timer;
  index::GraphIndex graph_index(16, 50);
  graph_index.build(store);
  std::cout << "      Graph build time: " << build_timer.elapsed_ms() << " ms\n";

  // 4. 执行搜索压测与 Recall 计算
  std::cout << "[4/4] Running Benchmark (" << q_count << " queries, TopK=" << topk << ")...\n";
  std::vector<double> latencies;
  latencies.reserve(q_count);
  double total_recall = 0.0;
  double total_graph_time = 0.0;

  for (size_t i = 0; i < q_count; ++i) {
    const float* query = store.get_vector(i % n);

    // A. 算真值 (不计入图索引的耗时)
    auto gt_res = flat_index.search(query, dim, topk);
    std::unordered_set<uint32_t> gt_set;
    for (const auto& neighbor : gt_res) {
      gt_set.insert(neighbor.internal_idx);
    }

    // B. 测试 GraphIndex
    utils::Timer q_timer;
    auto pred_res = graph_index.search(query, dim, topk);
    double cost = q_timer.elapsed_ms();  // 拿到单词耗时
    latencies.push_back(cost);
    total_graph_time += cost;  // 累加真实耗时

    // C. 算 Recall
    size_t hit = 0;
    for (const auto& neighbor : pred_res) {
      if (gt_set.count(neighbor.internal_idx)) {
        hit++;
      }
    }
    total_recall += static_cast<double>(hit) / topk;
  }

  // 用真实的图搜索总耗时计算指标
  std::sort(latencies.begin(), latencies.end());
  double avg_recall = total_recall / q_count;
  double avg_latency = total_graph_time / q_count;
  double p95_latency = latencies[static_cast<size_t>(q_count * 0.95)];
  double qps = q_count / (total_graph_time / 1000.0);

  std::cout << "\n=== GraphIndex Benchmark Results ===\n";
  std::cout << "Recall@" << topk << "   : " << (avg_recall * 100.0) << " %\n";
  std::cout << "Avg Latency : " << avg_latency << " ms\n";
  std::cout << "P95 Latency : " << p95_latency << " ms\n";
  std::cout << "QPS         : " << qps << " queries/sec\n";
  std::cout << "====================================\n";

  return 0;
}