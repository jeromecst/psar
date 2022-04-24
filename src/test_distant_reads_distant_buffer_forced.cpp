#include "psar_common.h"

int main() {
	constexpr psar::BenchmarkReadsSimpleConfig config{
		.allow_migrations_during_reads = false,
		.buffer_location = psar::Location::OnPageCacheCore,
	};
	psar::benchmark_reads_simple(
		config, "results/test_distant_reads_distant_buffer_forced.json");
}
