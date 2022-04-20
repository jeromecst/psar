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

std::vector<char> make_read_buffer() {
  struct stat st {};
  if (stat(TestFileName, &st) != 0) {
    perror("stat");
    exit(1);
  }

  std::vector<char> buffer(st.st_size);
  std::fill(buffer.begin(), buffer.end(), 0xff);
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
  bitmask nodes{};
  numa_bitmask_clearall(&nodes);
  numa_bitmask_setbit(&nodes, node);
  numa_bind(&nodes);
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

void BenchmarkResult::add_measurements(unsigned int init_core,
                                       unsigned int read_core,
                                       std::vector<long> times) {
  measurements.push_back(Measurements{
      .init_core = init_core,
      .init_node = core_to_node(init_core),
      .read_core = read_core,
      .read_node = core_to_node(read_core),
      .times_us = std::move(times),
  });
}

using json = nlohmann::json;

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(BenchmarkResult::Measurements, read_core,
                                   read_node, init_core, init_node, times_us)

void BenchmarkResult::save(const std::string &output_file) {
  json root;
  root["measurements"] = measurements;
  std::ofstream o{output_file};
  o << root;
}

} // namespace psar
