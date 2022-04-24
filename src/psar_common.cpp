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

void benchmark_reads(const BenchmarkReadsConfig &config,
                     BenchmarkResult &result) {
	// fill the page cache -- one read is enough
	drop_caches();
	{
		setaffinity_node(config.pagecache_core);
		auto read_buffer = make_local_read_buffer();
		read_file(read_buffer.data(), read_buffer.size());
	}

	const auto do_benchmark = [&] {
		setaffinity_node(config.buffer_core);
		auto read_buffer = config.bind_read_buffer
		                       ? make_node_bound_read_buffer(config.buffer_core)
		                       : make_local_read_buffer();

		if (config.allow_migrations_during_reads)
			setaffinity_any();
		else
			setaffinity_node(config.read_core);

		std::vector<long> times(config.num_iterations);
		std::vector<unsigned int> nodes(config.num_iterations);
		for (int i = 0; i < config.num_iterations; ++i) {
			times[i] = time_us(
				[&] { read_file(read_buffer.data(), read_buffer.size()); });
			nodes[i] = get_current_node();
		}

		std::cout << " read_core=" << config.read_core
				  << " buffer_core=" << config.buffer_core
				  << " pagecache_core=" << config.pagecache_core << '\n';
		result.add_measurements(config.pagecache_core, config.read_core,
		                        std::move(times), std::move(nodes));
	};

	// start a new thread to ensure the internal NUMA balancing stats
	// are reset (as init_numa_balancing is called from sched_fork)
	std::thread thread(do_benchmark);
	thread.join();
}

void benchmark_reads_simple(const BenchmarkReadsSimpleConfig &config,
                            const std::string &output_file) {
	const auto num_nodes = get_num_nodes();

	BenchmarkResult result;
	result.measurements.reserve(num_nodes);

	BenchmarkReadsConfig config_{
		.allow_migrations_during_reads = config.allow_migrations_during_reads,
		.bind_read_buffer = false,
		.pagecache_core = config.pagecache_core,
		.num_iterations = config.num_iterations,
	};

	for (unsigned int node = 0; node < num_nodes; ++node) {
		config_.read_core = int(node);
		config_.buffer_core = [&] {
			switch (config.buffer_location) {
			case Location::OnReadCore:
				return config_.read_core;
			case Location::OnPageCacheCore:
				return config_.pagecache_core;
			default:
				throw std::invalid_argument("invalid buffer_location");
			}
		}();

		benchmark_reads(config_, result);
	}

	result.save(output_file);
}

void benchmark_reads_get_times(const BenchmarkGetTimesConfig &config,
                               BenchmarkResult &result) {
	const struct {
		int pagecache, read, buffer;
	} node_configurations[] = {
		// local pagecache / local buffer
		{config.node_a, config.node_a, config.node_a},
		// local pagecache / distant buffer
		{config.node_a, config.node_a, config.node_b},
		// distant pagecache / local buffer
		{config.node_a, config.node_b, config.node_b},
		// distant pagecache / distant buffer
		{config.node_a, config.node_b, config.node_a},
	};

	BenchmarkReadsConfig config_{
		.allow_migrations_during_reads = config.allow_migrations_during_reads,
		.bind_read_buffer = true,
		.num_iterations = config.num_iterations,
	};
	for (const auto &nodes : node_configurations) {
		config_.pagecache_core = nodes.pagecache;
		config_.read_core = nodes.read;
		config_.buffer_core = nodes.buffer;
		benchmark_reads(config_, result);
	}
}

void benchmark_reads_get_times(const BenchmarkGetTimesConfig &config,
                               const std::string &output_file) {
	BenchmarkResult result;
	result.measurements.reserve(4);
	benchmark_reads_get_times(config, result);
	result.save(output_file);
}

} // namespace psar
