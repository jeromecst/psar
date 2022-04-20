#pragma once

#include <chrono>
#include <cstdio>
#include <thread>
#include <vector>

namespace psar {

void drop_caches();

std::vector<char> make_read_buffer();

void read_file(char *buf, size_t buf_size);

void setaffinity(unsigned int core);
void setaffinity_any();

inline unsigned int get_num_cores() {
  return std::thread::hardware_concurrency();
}

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

} // namespace psar
