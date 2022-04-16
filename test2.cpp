#include <benchmark/benchmark.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <cstring>
#include <iostream>

/* buffer size is 256KiB */
#define SIZE 262144
#define NCPU 24

static void read_file(char *buf)
{
	struct stat st; 
	char buffer[SIZE];

	if (buf == nullptr) {
		buf = buffer;
	}
	if (stat("fichiertest", &st) != 0) {
		perror("stat");
		exit(1);
	}

	int fd = open("fichiertest", O_RDONLY);
	if(fd < 0) {
		perror("open");
		exit(1);
	}
	int sum = 0;
	int sz;
	while((sz = read(fd, buf, SIZE)) > 0) {
		sum += sz;
	}
	if (sum != st.st_size) {
		printf("erreur %d\n", sum);
		exit(1);
	}
	close(fd);
}

static void setaffinity(int ncore)
{
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(ncore, &cpuset);
	pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset);
}

static void setaffinity_any()
{
	unsigned int cpu;
	if (getcpu(&cpu, nullptr) < 0) {
		perror("getcpu");
		exit(1);
	}
	cpu_set_t *cpuset = CPU_ALLOC(NCPU);
	for (unsigned int i = 0; i < NCPU; i++) {
		CPU_SET(i, cpuset);
	}
	pthread_setaffinity_np(pthread_self(), sizeof(cpuset), cpuset);
}

static void BM_PageCache(benchmark::State& state)
{
	int fd = open("/proc/sys/vm/drop_caches", O_WRONLY);

	if(write(fd, "1", 1) <= 0) {
		perror("write");
	}
	close(fd);

	/* only one read will fill the page cache */
	/* read on CPU X */
	setaffinity(state.range(0));
	read_file(nullptr);

	/* allocate and fill a buffer on CPU Y */
	setaffinity(state.range(1));
	char *buf = new char[SIZE];
	memset(buf, 0, SIZE);
	for (int i = 0; i < SIZE; i++) {
		buf[i] = i;
	}
	
	setaffinity_any();

	/* unsigned int cpu, node; */

	for (auto _ : state) {
		read_file(buf);
	}
	delete[] buf;
}

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

BENCHMARK_MAIN();
