#include "psar_common.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <numa.h>
#include <pthread.h>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>
#include <utility>

#include <nlohmann/json.hpp>

namespace psar {

namespace {

struct NumaAvailableChecker {
	NumaAvailableChecker() {
		if (numa_available() == -1)
			throw std::runtime_error("numa_available() returned -1");
	}
};

NumaAvailableChecker s_numa_available_checker;

} // namespace

constexpr const char *TestFileName = "fichiertest";

void drop_caches() {
	int fd = open("/proc/sys/vm/drop_caches", O_WRONLY);
	if (write(fd, "1", 1) <= 0) {
		perror("write");
		throw std::runtime_error("failed to drop caches");
	}
	close(fd);
}

NumaBuffer::NumaBuffer(size_t size) {
	if (size == 0)
		return;

	// using malloc is fine here because glibc's malloc implementation
	// uses mmap for allocations above 0x20000 bytes -- and new pages are
	// allocated on the local node by default
	void *data = std::malloc(std::max<size_t>(size, 0x40000));
	if (data == nullptr)
		throw std::bad_alloc();

	buffer = {static_cast<char *>(data), size};
	deleter = [](NumaBuffer *self) { std::free(self->buffer.data()); };
}

NumaBuffer::NumaBuffer(unsigned int node, size_t size) {
	if (size == 0)
		return;

	void *data = numa_alloc_onnode(size, int(node));
	if (data == nullptr)
		throw std::bad_alloc();

	buffer = {static_cast<char *>(data), size};
	deleter = [](NumaBuffer *self) {
		numa_free(self->buffer.data(), self->buffer.size());
	};
}

NumaBuffer::~NumaBuffer() {
	if (buffer.data() != nullptr)
		deleter(this);
}

NumaBuffer make_local_read_buffer() {
	struct stat st {};
	if (stat(TestFileName, &st) != 0) {
		perror("stat");
		exit(1);
	}

	NumaBuffer buffer(st.st_size);
	std::fill(buffer.buffer.begin(), buffer.buffer.end(), 0xff);
	return buffer;
}

NumaBuffer make_node_bound_read_buffer(unsigned int node) {
	struct stat st {};
	if (stat(TestFileName, &st) != 0) {
		perror("stat");
		exit(1);
	}

	NumaBuffer buffer(node, st.st_size);
	std::fill(buffer.buffer.begin(), buffer.buffer.end(), 0xff);
	return buffer;
}

void read_file(char *buf, size_t size) {
	struct stat st {};

	if (stat(TestFileName, &st) != 0) {
		perror("stat");
		exit(1);
	}

	if (!buf || size != size_t(st.st_size))
		throw std::runtime_error("incorrect buffer size");

	int fd = open(TestFileName, O_RDONLY);
	if (fd < 0) {
		perror("open");
		exit(1);
	}
	ssize_t total = 0;
	ssize_t sz;
	while ((sz = read(fd, buf + total, size - total)) > 0) {
		total += sz;
	}
	if (total != st.st_size) {
		printf("erreur %zd\n", total);
		exit(1);
	}
	close(fd);
}

void setaffinity(unsigned int core) {
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(core, &cpuset);
	pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset);
}

void setaffinity_node(unsigned int node) {
	bitmask *bmp = numa_allocate_nodemask();
	numa_bitmask_setbit(bmp, node);
	numa_run_on_node_mask(bmp);
	numa_free_nodemask(bmp);
}

void setaffinity_any() {
	numa_bind(numa_all_nodes_ptr);
}

unsigned int get_num_cores() {
	return std::thread::hardware_concurrency();
}

unsigned int get_num_nodes() {
	return numa_num_configured_nodes();
}

static unsigned int core_to_node(unsigned int core) {
	int ret = numa_node_of_cpu(int(core));
	if (ret < 0)
		throw std::runtime_error("numa_node_of_cpu failed");

	return static_cast<unsigned int>(ret);
}

unsigned int get_current_node() {
	return core_to_node(sched_getcpu());
}

void BenchmarkResult::add_measurements(unsigned int init_core,
                                       unsigned int read_core,
                                       std::vector<long> times,
                                       std::vector<unsigned int> nodes) {
	measurements.push_back(Measurements{
		.init_core = init_core,
		.init_node = core_to_node(init_core),
		.read_core = read_core,
		.read_node = core_to_node(read_core),
		.times_us = std::move(times),
		.nodes = std::move(nodes),
	});
}

using json = nlohmann::json;

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(BenchmarkResult::Measurements, read_core,
                                   read_node, init_core, init_node, times_us,
                                   nodes)

void BenchmarkResult::save(const std::string &output_file) {
	json root;
	root["measurements"] = measurements;
	std::ofstream o{output_file};
	o << root;
}

} // namespace psar
