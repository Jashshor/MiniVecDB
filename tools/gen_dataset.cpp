#include <fstream>
#include <limits>
#include <iostream>
#include <random>
#include <string>
#include <vector>
#include <cstdint>

namespace {
void write_u64_le(std::ofstream& out, uint64_t value) {
  unsigned char bytes[8];
  for (int i = 0; i < 8; ++i) {
    bytes[i] = static_cast<unsigned char>((value >> (i * 8)) & 0xFFu);
  }
  out.write(reinterpret_cast<const char*>(bytes), sizeof(bytes));
}
}  // namespace

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
  std::string out_path = argv[3];
  std::mt19937 gen(42);
  std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

  std::ofstream out(out_path, std::ios::binary);
  if (!out.is_open()) {
    std::cerr << "[ERROR] Failed to open file: " << out_path << "\n";
    return 1;
  }

  // File header uses fixed 64-bit unsigned integers in little-endian byte order:
  // [n(uint64 LE)][dim(uint64 LE)] then raw float32 vectors.
  write_u64_le(out, n);
  write_u64_le(out, dim);
  const size_t dim_size = static_cast<size_t>(dim);
  std::vector<float> vec(dim_size);
  for (uint64_t i = 0; i < n; ++i) {
    for (size_t d = 0; d < dim_size; ++d) {
      vec[d] = dist(gen);
    }
    out.write(reinterpret_cast<const char*>(vec.data()), dim_size * sizeof(float));
  }
  out.close();
  std::cout << "[SUCCESS] Generated " << n << " vectors of dim " << dim << " to " << out_path
            << ".\n";
  return 0;
}
