#pragma once

#include <filesystem>
#include <iostream>

#include <benchmark/benchmark.h>

// delete everything in the parent of KTS_SQLITE_PREFIX that has the
// $KTS_SQLITE_PREFIX(anything).sqlite
inline static void delete_database() {
  namespace fs = std::filesystem;

  const char *rawPrefixPath = std::getenv("KTS_SQLITE_PREFIX");
  if (!rawPrefixPath) {
    return;
  }
  const fs::path prefixPath(fs::absolute(rawPrefixPath));
  const fs::path prefixDir = prefixPath.parent_path();
  if (!fs::exists(prefixDir) || !fs::is_directory(prefixDir)) {
    std::cerr << "Directory does not exist or is not a directory.\n";
    return;
  }

  const std::string prefix = prefixPath.filename().string();
  const std::string suffix = ".sqlite";
  for (const auto &entry : fs::directory_iterator(prefixDir)) {
    if (entry.is_regular_file()) {
      const auto &path = entry.path();
      const auto &filename = path.filename().string();

      if (filename.size() >= prefix.size() + suffix.size() &&
          filename.compare(0, prefix.size(), prefix) == 0 &&
          filename.compare(filename.size() - suffix.size(), suffix.size(),
                           suffix) == 0) {
        std::cerr << __FILE__ << ":" << __LINE__ << " remove " << path << "\n";
        fs::remove(path);
      }
    }
  }
}

#define KTS_BENCHMARK_MAIN()                                                   \
  int main(int argc, char **argv) {                                            \
    setenv("KTS_SQLITE_PREFIX", "kts_perf_test_", false);                      \
    delete_database();                                                         \
    Kokkos::initialize();                                                      \
    ::benchmark::Initialize(&argc, argv);                                      \
    ::benchmark::RunSpecifiedBenchmarks();                                     \
    ::benchmark::Shutdown();                                                   \
    Kokkos::finalize();                                                        \
    delete_database();                                                         \
    return 0;                                                                  \
  }
