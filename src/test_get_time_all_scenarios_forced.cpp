#include "psar_common.h"

int main(int argc, const char **argv) {
	psar::BenchmarkGetTimesAllConfig config{
		.allow_migrations_during_reads = false,
		.bind_read_buffer = false,
		.num_iterations = psar::get_num_iterations(argc, argv).value_or(1000),
	};
	psar::benchmark_reads_get_times_all_scenarios(
		config, "results/test_get_time_all_scenarios_forced.json");
}
