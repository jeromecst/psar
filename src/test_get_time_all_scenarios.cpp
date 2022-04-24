#include "psar_common.h"

int main() {
	psar::BenchmarkGetTimesAllConfig config{
		.num_iterations = 5000,
	};
	psar::benchmark_reads_get_times_all_scenarios(
		config, "results/test_get_time_all_scenarios.json");
}
