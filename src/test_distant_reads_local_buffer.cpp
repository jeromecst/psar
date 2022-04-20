#include "psar_common.h"

int main() {
  constexpr psar::BenchmarkReadsSimpleConfig config{
      .set_affinity_any = true,
      .buffer_location = psar::BufferLocation::OnLocalNode,
  };
  psar::benchmark_reads_simple<config>(
      "results/test_distant_reads_distant_buffer.json");
}
