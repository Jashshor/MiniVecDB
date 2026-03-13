#pragma once
#include <random>
#include <vector>
#include "src/index/iindex.h"

namespace minivecdb {
namespace index {
struct HnswNode {
  int level;                                     // 该节点在 HNSW 图中所处的最高层数
  std::vector<std::vector<uint32_t>> neighbors;  // 存储该节点在每一层的邻居列表（索引为层数）
};

class HnswIndex : public IIndex {
 public:
  HnswIndex(size_t M = 16, size_t ef_construction = 100, size_t ef_search = 50, uint32_t seed = 42);
  void build(const storage::VectorStore& store) override;
  std::vector<Neighbor> search(const float* query, size_t dim, size_t topk) const override;

 private:
  const storage::VectorStore* store_ = nullptr;  // 指向底层向量数据集的指针
  size_t M_;                // 每一层每个节点的最大连接数容量 (M)
  size_t ef_construction_;  // 构建图时用于控制连边质量的动态候选列表大小，创建边调用
  size_t ef_search_;  // 搜索时用于控制召回率的动态候选列表大小，用户搜索时调用，兜底召回率

  double mult_;                   // 随机层数生成的概率控制乘子，通常为 1 / ln(M)
  std::mt19937 level_generator_;  // 用于生成节点层数的随机数生成器
  int random_level();

  std::vector<HnswNode> nodes_;  // 存储图中所有节点的拓扑信息（层级、各层的邻居）
  int max_level_;                // 当前整个 HNSW 图中存在的最高层级
  uint32_t entry_point_;         // 搜索的全局入口点节点 ID（通常在最高层级）

  // 用于在搜索过程中进行快速的“是否已访问”的判断
  mutable std::vector<uint32_t> visited_tag_;  // 记录每个节点上次被访问时的标记值
  mutable uint32_t current_tag_;               // 当前轮次搜索的标记值

  std::vector<Neighbor> search_layer(const float* query, uint32_t ep, size_t ef, int layer) const;
  std::vector<uint32_t> SelectNeighbors(const std::vector<Neighbor>& candidates, size_t M) const;
};
}  // namespace index
}  // namespace minivecdb