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
#define NUMA 4
/* buffer size is 256KiB */
#define SIZE 262144

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

static void setaffinity_node(int node)
{
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	for (int i = node; i < NCPU; i += NUMA) {
		CPU_SET(i, &cpuset);
	}
	pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset);
}

/* static void setaffinity(int ncore) */
/* { */
/* 	cpu_set_t cpuset; */
/* 	CPU_ZERO(&cpuset); */
/* 	CPU_SET(ncore, &cpuset); */
/* 	pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset); */
/* } */

/* overloaded with 2 args *1/
/* static void setaffinity(int ncore, int ncore2) */
/* { */
/* 	cpu_set_t cpuset; */
/* 	CPU_ZERO(&cpuset); */
/* 	CPU_SET(ncore, &cpuset); */
/* 	CPU_SET(ncore2, &cpuset); */
/* 	pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset); */
/* } */

static void setaffinity_any()
{
	unsigned int cpu;
	cpu_set_t cpuset;
	for (unsigned int i = 0; i < NCPU; i++) {
		CPU_SET(i, &cpuset);
	}
	pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset);
}

static void CustomArguments(benchmark::internal::Benchmark* b) {
	for (int j = 0; j < NUMA; j += 1)
		b->Args({0, j});
}

