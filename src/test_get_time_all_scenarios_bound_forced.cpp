#include "psar_common.h"

int main() {
	psar::BenchmarkGetTimesAllConfig config{
		.allow_migrations_during_reads = false,
		.bind_read_buffer = true,
		.num_iterations = 1000,
	};
	psar::benchmark_reads_get_times_all_scenarios(
		config, "results/test_get_time_all_scenarios_bound_forced.json");
}
