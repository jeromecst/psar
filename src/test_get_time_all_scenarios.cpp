#include "psar_common.h"

int main() {
	psar::BenchmarkGetTimesAllConfig config{
		.allow_migrations_during_reads = true,
		.bind_read_buffer = false,
		.num_iterations = 2000,
	};
	psar::benchmark_reads_get_times_all_scenarios(
		config, "results/test_get_time_all_scenarios.json");
}
