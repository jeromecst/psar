#include "psar_common.h"
#include <iostream>

namespace psar {

static void benchmark() {
  constexpr int InitCore = 0;
  constexpr int NumIterations = 300;
  const auto num_nodes = get_num_nodes();

  BenchmarkResult result;
  result.measurements.reserve(num_nodes);

  // fill the page cache on core InitCore -- one read is enough
  drop_caches();
  auto read_buffer = make_read_buffer();
  setaffinity_node(InitCore);
  read_file(read_buffer.data(), read_buffer.size());

  for (unsigned int node = 0; node < num_nodes; ++node) {
    setaffinity_any();

    std::vector<long> times(NumIterations);
    for (int i = 0; i < NumIterations; ++i) {
      times[i] =
          time_us([&] { read_file(read_buffer.data(), read_buffer.size()); });
    }

    std::cout << InitCore << '/' << node << '\n';
    result.add_measurements(InitCore, node, std::move(times));
  }

  result.save("results/test2.json");
}

} // namespace psar

int main() {
  psar::benchmark();
}
