#include <iostream>
#include <string>
#include "src/index/flat_index.h"
#include "src/storage/serializer.h"
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
  LOG_INFO("=== MiniVecDB Cold Start ===");
  utils::Timer load_timer;
  storage::VectorStore store = storage::Serializer::load(file_path);
  LOG_INFO("Loaded " << store.size() << " vectors (Dim: " << store.dim() << ") in "
                     << load_timer.elapsed_ms() << " ms.");
  LOG_INFO("Building FlatIndex...");
  index::FlatIndex flat_index;
  flat_index.build(store);
  LOG_INFO("Searching Top " << topk << "...");
  utils::Timer search_timer;
  const float* query = store.get_vector(0);
  auto results = flat_index.search(query, store.dim(), topk);

  LOG_INFO("Search completed in " << search_timer.elapsed_ms() << " ms.");
  for (size_t i = 0; i < results.size(); ++i) {
    std::cout << "Rank " << i + 1 << ": ID " << store.get_id(results[i].internal_idx)
              << ", Distance = " << results[i].distance << "\n";
  }

  return 0;
}
