#pragma once

#include <chrono>
#include <cstdio>
#include <iostream>
#include <span>
#include <string>
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
  }
  NumaBuffer &operator=(NumaBuffer &&other) noexcept {
    if (this != &other)
      buffer = std::exchange(other.buffer, {});
    return *this;
  }

  auto data() const { return buffer.data(); }
  auto size() const { return buffer.size(); }

  std::span<char> buffer;
};

void drop_caches();

NumaBuffer make_read_buffer(unsigned int node);
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
    /// the active node at the end of each sample
    std::vector<unsigned int> nodes;
  };

  void add_measurements(unsigned int init_core, unsigned int read_core,
                        std::vector<long> times,
                        std::vector<unsigned int> nodes);

  void save(const std::string &output_file);

  std::vector<Measurements> measurements;
};

enum class BufferLocation {
  OnInitNode,
  OnLocalNode,
};

struct BenchmarkReadsSimpleConfig {
  bool set_affinity_any = false;
  BufferLocation buffer_location = BufferLocation::OnLocalNode;
  int init_core = 0;
  int num_iterations = 2000;
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

  for (unsigned int node = 0; node < num_nodes; ++node) {
    setaffinity_node(node);

    auto read_buffer = [] {
      if constexpr (config.buffer_location == BufferLocation::OnLocalNode) {
        return make_local_read_buffer();
      } else if constexpr (config.buffer_location ==
                           BufferLocation::OnInitNode) {
        return make_read_buffer(config.init_core);
      }
    }();

    if constexpr (config.set_affinity_any)
      setaffinity_any();

    std::vector<long> times(config.num_iterations);
    std::vector<unsigned int> nodes(config.num_iterations);
    for (int i = 0; i < config.num_iterations; ++i) {
      times[i] =
          time_us([&] { read_file(read_buffer.data(), read_buffer.size()); });
      nodes[i] = get_current_node();
    }

    std::cout << config.init_core << '/' << node << '\n';
    result.add_measurements(config.init_core, node, std::move(times),
                            std::move(nodes));
  }

  result.save(output_file);
}

} // namespace psar
