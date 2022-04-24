#pragma once

#include <chrono>
#include <cstdio>
#include <iostream>
#include <span>
#include <string>
#include <thread>
#include <vector>

namespace psar {

struct NumaBuffer {
	explicit NumaBuffer(size_t size);
	NumaBuffer(unsigned int node, size_t size);
	~NumaBuffer();
	NumaBuffer(const NumaBuffer &) = delete;
	auto operator=(const NumaBuffer &) = delete;
	NumaBuffer(NumaBuffer &&other) noexcept {
		buffer = std::exchange(other.buffer, {});
		deleter = other.deleter;
	}
	NumaBuffer &operator=(NumaBuffer &&other) noexcept {
		if (this != &other) {
			buffer = std::exchange(other.buffer, {});
			deleter = other.deleter;
		}
		return *this;
	}

	auto data() const { return buffer.data(); }
	auto size() const { return buffer.size(); }

	std::span<char> buffer;
	void (*deleter)(NumaBuffer *buffer){};
};

void drop_caches();

NumaBuffer make_node_bound_read_buffer(unsigned int node);
NumaBuffer make_local_read_buffer();

void read_file(char *buf, size_t buf_size);

void setaffinity(unsigned int core);
void setaffinity_node(unsigned int node);
void setaffinity_any();

unsigned int get_num_cores();
unsigned int get_num_nodes();
unsigned int get_current_node();

template <typename Fn> long time_us(const Fn &fn) {
	// libstdc++ and libc++ both use clock_gettime(CLOCK_MONOTONIC) on POSIX
	auto start = std::chrono::steady_clock::now();
	fn();
	auto elapsed = std::chrono::steady_clock::now() - start;
	return std::chrono::duration_cast<std::chrono::microseconds>(elapsed)
	    .count();
}

enum class Location {
	OnPageCacheCore,
	OnReadCore,
};

struct BenchmarkReadsConfig {
	bool allow_migrations_during_reads = true;
	/// whether the read buffer should be bound to buffer_core
	/// (if true, the buffer will never be migrated automatically)
	bool bind_read_buffer = false;
	int pagecache_core = 0;
	int read_core = 0;
	int buffer_core = 0;
	int num_iterations = 1000;
};

struct BenchmarkResult {
	struct Measurements {
		unsigned int init_core;
		unsigned int init_node;
		unsigned int read_core;
		unsigned int read_node;
		unsigned int buffer_core;
		unsigned int buffer_node;
		/// times in μs
		std::vector<long> times_us;
		/// the active node at the end of each sample
		std::vector<unsigned int> nodes;
	};

	void add_measurements(const BenchmarkReadsConfig &config,
	                      std::vector<long> times,
	                      std::vector<unsigned int> nodes);

	void save(const std::string &output_file);

	std::vector<Measurements> measurements;
};

void benchmark_reads(const BenchmarkReadsConfig &config,
                     BenchmarkResult &result);

struct BenchmarkReadsSimpleConfig {
	bool allow_migrations_during_reads = true;
	Location buffer_location = Location::OnReadCore;
	int pagecache_core = 0;
	int num_iterations = 1000;
};

void benchmark_reads_simple(const BenchmarkReadsSimpleConfig &config,
                            const std::string &output_file);

struct BenchmarkGetTimesConfig {
	bool allow_migrations_during_reads = false;
	int node_a = 1;
	int node_b = 2;
	int num_iterations = 1000;
};

void benchmark_reads_get_times(const BenchmarkGetTimesConfig &config,
                               BenchmarkResult &result);

void benchmark_reads_get_times(const BenchmarkGetTimesConfig &config,
                               const std::string &output_file);

void benchmark_reads_get_times_all_scenarios(const std::string &output_file);

} // namespace psar
