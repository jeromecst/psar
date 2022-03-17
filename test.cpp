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
	for(int i = 0; i < 100; i++) {
		read_file();
	}

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

BENCHMARK_MAIN();
