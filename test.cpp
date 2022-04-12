#include <benchmark/benchmark.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>


static void read_file()
{
	struct stat st; 
	if (stat("fichiertest", &st) != 0) {
		perror("stat");
		exit(1);
	}

	int fd = open("fichiertest", O_RDONLY);
	if(fd < 0) {
		perror("open");
		exit(1);
	}
	char * buf = new char[4096];
	int sum = 0;
	int sz;
	while((sz = read(fd, buf, 4096)) > 0) {
		sum += sz;
	}
	if (sum != st.st_size) {
		printf("erreur %d\n", sum);
		exit(1);
	}
	delete[] buf;
	close(fd);
}

static void setaffinity(int ncore)
{
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(ncore, &cpuset);
	pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset);
}

static void BM_PageCache(benchmark::State& state)
{
	int fd = open("/proc/sys/vm/drop_caches", O_WRONLY);
	if(write(fd, "1", 1) <= 0) {
		perror("write");
	}
	close(fd);

	setaffinity(state.range(0));
	/* only one read will fill the page cache */
	read_file();

	setaffinity(state.range(1));
	for (auto _ : state) {
		read_file();
	}
}

BENCHMARK(BM_PageCache)
    ->Args({0, 0})
    ->Args({0, 1})
    ->Args({0, 2})
    ->Args({0, 3})
    ->Unit(benchmark::kMillisecond)
;

/*
BENCHMARK(BM_PageCache)
    ->Args({0, 0})
    ->Args({0, 2})
    ->Args({0, 4})
    ->Args({0, 6})
    ->Args({0, 8})
    ->Args({0, 10})
    ->Args({0, 12})
    ->Args({0, 14})
    ->Args({0, 16})
    ->Args({0, 18})
    ->Args({0, 22})
    ->Args({0, 1})
    ->Args({0, 3})
    ->Args({0, 5})
    ->Args({0, 7})
    ->Args({0, 9})
    ->Args({0, 11})
    ->Args({0, 13})
    ->Args({0, 15})
    ->Args({0, 17})
    ->Args({0, 19})
    ->Args({0, 21})
    ->Args({0, 23})
    ->Unit(benchmark::kMillisecond)
;
*/

BENCHMARK_MAIN();
