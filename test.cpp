#include <benchmark/benchmark.h>
#include "test.h"

#define NCPU 127
#define SIZE 32768

static void BM_PageCache(benchmark::State& state)
{
	int fd = open("/proc/sys/vm/drop_caches", O_WRONLY);
	if(write(fd, "1", 1) <= 0) {
		perror("write");
	}
	close(fd);

	setaffinity(state.range(0));
	/* only one read will fill the page cache */
	read_file(nullptr, SIZE);

	setaffinity(state.range(1));
	for (auto _ : state) {
		read_file(nullptr, SIZE);
	}
}

BENCHMARK(BM_PageCache)
->Apply(CustomArguments)
->Unit(benchmark::kMillisecond)
;

BENCHMARK_MAIN();
