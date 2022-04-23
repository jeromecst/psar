#include "psar_common.h"

int main() {
	constexpr psar::BenchmarkReadsSimpleConfig config{
		.set_affinity_any = false,
		.buffer_location = psar::Location::OnLocalNode,
		.local_node = 1,
		.distant_node = 2,
	};
	psar::benchmark_reads_get_times<config>(
		"results/test_get_time.json");
}
