#pragma once

#include <chrono>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

namespace psar {

void drop_caches();

std::vector<char> make_read_buffer();

void read_file(char *buf, size_t buf_size);

void setaffinity(unsigned int core);
void setaffinity_node(unsigned int node);
void setaffinity_any();

unsigned int get_num_cores();
unsigned int get_num_nodes();

template <typename Fn> long time_us(const Fn &fn) {
  // libstdc++ and libc++ both use clock_gettime(CLOCK_MONOTONIC) on POSIX
  auto start = std::chrono::steady_clock::now();
  fn();
  auto elapsed = std::chrono::steady_clock::now() - start;
  return std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
}

struct BenchmarkResult {
  struct Measurements {
    unsigned int init_core;
    unsigned int init_node;
    unsigned int read_core;
    unsigned int read_node;
    /// times in us
    std::vector<long> times_us;
  };

  void add_measurements(unsigned int init_core, unsigned int read_core,
                        std::vector<long> times);

  void save(const std::string &output_file);

  std::vector<Measurements> measurements;
};

struct BenchmarkReadsSimpleConfig {
  bool set_affinity_any = false;
  int init_core = 0;
  int num_iterations = 300;
};

template <BenchmarkReadsSimpleConfig config>
inline void benchmark_reads_simple(const std::string &output_file) {
  const auto num_nodes = get_num_nodes();

  BenchmarkResult result;
  result.measurements.reserve(num_nodes);

  // fill the page cache on core InitCore -- one read is enough
  drop_caches();
  auto read_buffer = make_read_buffer();
  setaffinity_node(config.init_core);
  read_file(read_buffer.data(), read_buffer.size());

  for (unsigned int node = 0; node < num_nodes; ++node) {
    if constexpr (config.set_affinity_any)
      setaffinity_any();
    else
      setaffinity_node(node);

    std::vector<long> times(config.num_iterations);
    for (int i = 0; i < config.num_iterations; ++i) {
      times[i] =
          time_us([&] { read_file(read_buffer.data(), read_buffer.size()); });
    }

    std::cout << config.init_core << '/' << node << '\n';
    result.add_measurements(config.init_core, node, std::move(times));
  }

  result.save(output_file);
}

} // namespace psar
