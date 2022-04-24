#include "psar_common.h"

int main() {
	constexpr psar::BenchmarkReadsSimpleConfig config{
		.allow_migrations_during_reads = true,
		.buffer_location = psar::Location::OnReadCore,
	};
	psar::benchmark_reads_simple(
		config, "results/test_distant_reads_local_buffer.json");
}
