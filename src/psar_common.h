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

struct BenchmarkResult {
	struct Measurements {
		unsigned int init_core;
		unsigned int init_node;
		unsigned int read_core;
		unsigned int read_node;
		/// times in us
		std::vector<long> times_us;
		/// the active node at the end of each sample
		std::vector<unsigned int> nodes;
	};

	void add_measurements(unsigned int init_core, unsigned int read_core,
	                      std::vector<long> times,
	                      std::vector<unsigned int> nodes);

	void save(const std::string &output_file);

	std::vector<Measurements> measurements;
};

enum class Location {
	OnInitNode,
	OnDistantNode,
	OnLocalNode,
};

struct BenchmarkReadsSimpleConfig {
	bool set_affinity_any = false;
	Location buffer_location = Location::OnLocalNode;
	Location pagecache_location = Location::OnLocalNode;
	int init_core = 0;
	int local_node = 0;
	int distant_node = 0;
	int num_iterations = 1000;
};

template <BenchmarkReadsSimpleConfig config>
inline void benchmark_reads_simple(const std::string &output_file) {
	const auto num_nodes = get_num_nodes();

	BenchmarkResult result;
	result.measurements.reserve(num_nodes);

	// fill the page cache on core InitCore -- one read is enough
	drop_caches();
	{
		setaffinity_node(config.init_core);
		auto read_buffer = make_local_read_buffer();
		read_file(read_buffer.data(), read_buffer.size());
	}

	const auto benchmark_node = [&](unsigned int node) {
		setaffinity_node(node);

		auto read_buffer = [] {
			if constexpr (config.buffer_location == Location::OnLocalNode) {
				return make_local_read_buffer();
			} else if constexpr (config.buffer_location ==
			                     Location::OnInitNode) {
				return make_node_bound_read_buffer(config.init_core);
			}
		}();

		if constexpr (config.set_affinity_any)
			setaffinity_any();

		std::vector<long> times(config.num_iterations);
		std::vector<unsigned int> nodes(config.num_iterations);
		for (int i = 0; i < config.num_iterations; ++i) {
			times[i] = time_us(
				[&] { read_file(read_buffer.data(), read_buffer.size()); });
			nodes[i] = get_current_node();
		}

		std::cout << config.init_core << '/' << node << '\n';
		result.add_measurements(config.init_core, node, std::move(times),
		                        std::move(nodes));
	};

	for (unsigned int node = 0; node < num_nodes; ++node) {
		// start a new thread to ensure the internal NUMA balancing stats
		// are reset (as init_numa_balancing is called from sched_fork)
		std::thread thread(benchmark_node, node);

		// wait for the benchmark to complete before moving on to the next node
		thread.join();
	}

	result.save(output_file);
}

template <BenchmarkReadsSimpleConfig config>
inline void benchmark_reads_get_times(const std::string &output_file) {
	const auto num_nodes = get_num_nodes();
	const Location listlocationpc[] = {Location::OnLocalNode,
	                                   Location::OnDistantNode};
	const Location listlocationbuff[] = {Location::OnLocalNode,
	                                     Location::OnDistantNode};
	int pagecache_node;

	BenchmarkResult result;
	result.measurements.reserve(num_nodes);

	for (auto PCLocation : listlocationpc) {
		for (auto BuffLocation : listlocationbuff) {
			/* fill the page cache -- one read is enough */
			drop_caches();
			{
				auto read_buffer = [&] {
					if (PCLocation == Location::OnLocalNode) {
						pagecache_node = config.local_node;
						setaffinity_node(pagecache_node);
						return make_local_read_buffer();
					} else if (PCLocation == Location::OnDistantNode) {
						pagecache_node = config.distant_node;
						setaffinity_node(pagecache_node);
						return make_node_bound_read_buffer(config.distant_node);
					}
					exit();
				}();
				read_file(read_buffer.data(), read_buffer.size());
			}

			const auto benchmark_node = [&](unsigned int node) {
				setaffinity_node(node);

				auto read_buffer = [&] {
					if (BuffLocation == Location::OnLocalNode) {
						return make_local_read_buffer();
					} else if (BuffLocation == Location::OnDistantNode) {
						return make_node_bound_read_buffer(config.distant_node);
					}
					exit();
				}();

				if constexpr (config.set_affinity_any)
					setaffinity_any();

				std::vector<long> times(config.num_iterations);
				std::vector<unsigned int> nodes(config.num_iterations);
				for (int i = 0; i < config.num_iterations; ++i) {
					times[i] = time_us([&] {
						read_file(read_buffer.data(), read_buffer.size());
					});
					nodes[i] = get_current_node();
				}

				std::cout << pagecache_node << '/' << node << '\n';
				result.add_measurements(pagecache_node, node, std::move(times),
				                        std::move(nodes));
			};

			// start a new thread to ensure the internal NUMA balancing
			// stats are reset (as init_numa_balancing is called from
			// sched_fork)
			// always schedule on local node
			std::thread thread(benchmark_node, config.local_node);

			// wait for the benchmark to complete before moving on to the
			// next node
			thread.join();
		}

		result.save(output_file);
	}
}

} // namespace psar
