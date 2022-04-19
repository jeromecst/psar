#include <stdio.h>
#include <benchmark/benchmark.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <cstring>
#include <iostream>

#define NCPU 128

static void read_file(char *buf, ssize_t size)
{
	struct stat st; 
	char buffer[size];

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
	while((sz = read(fd, buf, size)) > 0) {
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

static void CustomArguments(benchmark::internal::Benchmark* b) {
	for (int j = 0; j < NCPU; j += 1)
		b->Args({0, j});
}

