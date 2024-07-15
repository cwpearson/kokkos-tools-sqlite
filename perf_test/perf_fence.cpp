#include <Kokkos_Core.hpp>

#include "perf_main.hpp"


static void BM_fence(benchmark::State& state) {
  for (auto _ : state) {
    Kokkos::fence();
  }
}
// Register the function as a benchmark
BENCHMARK(BM_fence);

KTS_BENCHMARK_MAIN();