#pragma once

#include <chrono>
#include <cstdio>
#include <iostream>
#include <span>
#include <string>
#include <thread>
#include <vector>

namespace psar {

/// A dynamically allocated contiguous buffer that is suitable for
/// NUMA experiments.
struct NumaBuffer {
	/// Allocates a buffer of `size` bytes using malloc.
	/// The buffer is guaranteed to be allocated in a fresh page.
	explicit NumaBuffer(size_t size);

	/// Allocates a buffer of `size` bytes on the specified node
	/// using numa_alloc_onnode.
	/// The buffer is guaranteed to be allocated in a fresh page.
	NumaBuffer(unsigned int node, size_t size);

	/// Frees the buffer.
	~NumaBuffer();

	/// Copies are not allowed.
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

/// Drop the page cache. Requires root permissions.
void drop_caches();

/// Make a buffer that is as large as the test file
/// and bind it to the specified node (so that it cannot be migrated).
NumaBuffer make_node_bound_read_buffer(unsigned int node,
                                       const std::string &test_file_name);

/// Make a buffer that is as large as the test file.
/// Migrations are allowed.
NumaBuffer make_local_read_buffer(const std::string &test_file_name);

/// Read the test file into the specified buffer,
/// using sequential reads of 0x2000 bytes each.
/// @param buf_size Buffer size. Must match the file size.
void read_file(const std::string &name, char *buf, size_t buf_size);
/// Read the test file into the specified buffer,
/// using random reads (read + random seek) of 0x2000 bytes each.
/// @param buf_size Buffer size. Must match the file size.
void read_file_random(const std::string &name, char *buf, size_t buf_size);

/// Pin the current thread to the specified core.
void setaffinity(unsigned int core);
/// Pin the current thread to the specified NUMA node.
void setaffinity_node(unsigned int node);
/// Unpin the current thread.
void setaffinity_any();

/// Returns the number of CPU cores.
unsigned int get_num_cores();
/// Returns the number of NUMA nodes.
unsigned int get_num_nodes();
/// Returns the current NUMA node. Very fast.
unsigned int get_current_node();

/// Calls the specified function and returns the amount
/// of real time it took to run (in microseconds).
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
	/// whether the scheduler should be allowed to migrate the thread
	bool allow_migrations_during_reads = true;
	/// whether the read buffer should be bound to buffer_core
	/// (if true, the buffer will never be migrated automatically)
	bool bind_read_buffer = false;

	/// perform random reads (read random chunks of ReadChunkSize bytes)
	bool random_reads = false;
	/// number of times the file should be read in its entirety
	int num_iterations = 5000;
	/// name of the test file
	std::string test_file_name = "fichiertest";

	/// core on which the file should be loaded into the page cache
	int pagecache_core = 0;
	/// core on which reads should be done (initially; the thread might migrate
	/// to another core if migrations are allowed)
	int read_core = 0;
	/// core on which the buffer should be allocated (initially; the buffer
	/// might migrate to another node if it is not bound)
	int buffer_core = 0;
};

/// A BenchmarkResult is a series of measurements.
struct BenchmarkResult {
	/// A Measurements is a series of data points for a given set of initial
	/// placement parameters (see BenchmarkReadsConfig).
	struct Measurements {
		/// See BenchmarkReadsConfig for more information on the meaning
		/// of these parameters.
		unsigned int pagecache_core;
		unsigned int pagecache_node;
		unsigned int read_core;
		unsigned int read_node;
		unsigned int buffer_core;
		unsigned int buffer_node;

		/// times in μs
		std::vector<long> times_us;
		/// the active node at the end of each sample
		std::vector<unsigned int> nodes;
		/// the nodes on which the read buffer is allocated at the end of a read
		std::vector<unsigned int> buffer_nodes;
	};

	void add_measurements(const BenchmarkReadsConfig &config,
	                      std::vector<long> times,
	                      std::vector<unsigned int> nodes,
	                      std::vector<unsigned int> buffer_nodes);

	void save(const std::string &output_file);

	std::vector<Measurements> measurements;
};

/// Execute benchmarks with the specified config.
/// Measurements are added to `result`.
void benchmark_reads(const BenchmarkReadsConfig &config,
                     BenchmarkResult &result);

/// See BenchmarkReadsConfig for the meaning of these parameters.
struct BenchmarkReadsSimpleConfig {
	bool allow_migrations_during_reads = true;
	Location buffer_location = Location::OnReadCore;
	int pagecache_core = 0;
	int num_iterations = 5000;
};

void benchmark_reads_simple(const BenchmarkReadsSimpleConfig &config,
                            const std::string &output_file);

/// See BenchmarkReadsConfig for the meaning of these parameters.
struct BenchmarkGetTimesConfig {
	bool allow_migrations_during_reads = false;
	int node_a = 1;
	int node_b = 2;
	int num_iterations = 5000;
};

/// Measure read times in various situations (local page cache / local buffer,
/// local/remote, remote/local, remote/remote) with the specified config.
/// Measurements are added to `result`.
void benchmark_reads_get_times(const BenchmarkGetTimesConfig &config,
                               BenchmarkResult &result);

/// Measure read times in various situations (local page cache / local buffer,
/// local/remote, remote/local, remote/remote) with the specified config.
/// Measurements are written the specified output file.
void benchmark_reads_get_times(const BenchmarkGetTimesConfig &config,
                               const std::string &output_file);

/// See BenchmarkReadsConfig for the meaning of these parameters.
struct BenchmarkGetTimesAllConfig {
	bool allow_migrations_during_reads = true;
	bool bind_read_buffer = false;
	bool random_reads = false;
	int num_iterations = 5000;
	std::string test_file_name = "fichiertest";
};

/// Measure read times and locality information with the specified config.
/// Measurements are written the specified output file.
void benchmark_reads_get_times_all_scenarios(
	const BenchmarkGetTimesAllConfig &config, const std::string &output_file);

/// Initialises `config` from command line arguments.
void parse_get_times_all_config(BenchmarkGetTimesAllConfig *config, int argc,
                                const char **argv);

} // namespace psar
