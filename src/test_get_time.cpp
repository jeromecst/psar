#include "psar_common.h"

int main() {
	constexpr psar::BenchmarkGetTimesConfig config{
		.node_a = 1,
		.node_b = 2,
	};
	psar::benchmark_reads_get_times(config, "results/test_get_time.json");
}
