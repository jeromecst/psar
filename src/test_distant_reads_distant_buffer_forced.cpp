#include "psar_common.h"

int main() {
	constexpr psar::BenchmarkReadsSimpleConfig config{
		.set_affinity_any = false,
		.buffer_location = psar::BufferLocation::OnInitNode,
	};
	psar::benchmark_reads_simple<config>(
		"results/test_distant_reads_distant_buffer_forced.json");
}
