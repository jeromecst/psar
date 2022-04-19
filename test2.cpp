#include <benchmark/benchmark.h>
#include "test.h"

static void BM_PageCache(benchmark::State& state)
{
	int fd = open("/proc/sys/vm/drop_caches", O_WRONLY);

	if(write(fd, "1", 1) <= 0) {
		perror("write");
	}
	close(fd);

	/* only one read will fill the page cache */
	/* read on CPU X (only NUMA node 0) */
	setaffinity_node(state.range(0));
	read_file(nullptr);

	/* allocate and fill a buffer on CPU Y */
	setaffinity_node(state.range(1));
	char *buf = new char[SIZE];
	memset(buf, 0, SIZE);
	for (int i = 0; i < SIZE; i++) {
		buf[i] = i;
	}
	
	setaffinity_any();

	for (auto _ : state) {
		read_file(buf);
	}
	delete[] buf;
}

BENCHMARK(BM_PageCache)
->Apply(CustomArguments)
->Unit(benchmark::kMillisecond)
;

BENCHMARK_MAIN();
