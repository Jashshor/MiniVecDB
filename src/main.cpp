#include <iostream>
#include <vector>
#include "src/storage/vector_store.h"
#include "src/utils/logging.h"
#include "src/utils/timer.h"

using namespace minivecdb;

int main() {
  std::cout << "[INFO] MiniVecDB boot ok. Engine starting..." << std::endl;

  // 简单启动示例：计时 + 构建一个 dim=4 的 VectorStore。
  utils::Timer timer;
  storage::VectorStore store(4);
  store.reserve(100);

  std::vector<float> vec1 = {0.1f, 0.2f, 0.3f, 0.4f};
  std::vector<float> vec2 = {0.5f, 0.6f, 0.7f, 0.8f};

  // 插入两条向量，拿到第二条的内部索引。
  store.add(1001, vec1.data(), vec1.size());
  auto idx2 = store.add(1002, vec2.data(), vec2.size());
  LOG_INFO("Inserted 2 vectors. Store size: " << store.size());

  // 按内部索引读取向量首地址并打印内容。
  const float* ptr = store.get_vector(idx2);
  LOG_INFO("Read vector 2: [" << ptr[0] << ", " << ptr[1] << ", " << ptr[2] << ", " << ptr[3]
                              << "]");

  LOG_INFO("Total time elapsed: " << timer.elapsed_ms() << " ms");
  return 0;
}
