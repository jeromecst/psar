#include "psar_common.h"

int main() {
  constexpr psar::BenchmarkReadsSimpleConfig config{
      .set_affinity_any = true,
  };
  psar::benchmark_reads_simple<config>("results/test2.json");
}
