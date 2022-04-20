#include "psar_common.h"

int main() {
  constexpr psar::BenchmarkReadsSimpleConfig config{
      .set_affinity_any = false,
  };
  psar::benchmark_reads_simple<config>("results/test1.json");
}
