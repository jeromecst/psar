#include "psar_common.h"
#include <iostream>

namespace psar {

static void benchmark() {
  constexpr int InitCore = 0;
  constexpr int NumIterations = 300;
  const auto num_cores = get_num_cores();

  BenchmarkResult result;
  result.measurements.reserve(num_cores);

  // fill the page cache on core InitCore -- one read is enough
  drop_caches();
  auto read_buffer = make_read_buffer();
  setaffinity(InitCore);
  read_file(read_buffer.data(), read_buffer.size());

  for (unsigned int core = 0; core < num_cores; ++core) {
    setaffinity(core);

    std::vector<long> times(NumIterations);
    for (int i = 0; i < NumIterations; ++i) {
      times[i] =
          time_us([&] { read_file(read_buffer.data(), read_buffer.size()); });
    }

    std::cout << InitCore << '/' << core << '\n';
    result.add_measurements(InitCore, core, std::move(times));
  }

  result.save("results/test1.json");
}

} // namespace psar

int main() {
  psar::benchmark();
}
