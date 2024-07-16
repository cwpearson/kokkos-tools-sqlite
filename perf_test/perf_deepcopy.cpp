#include <Kokkos_Core.hpp>

#include "perf_main.hpp"


static void BM_deepcopy(benchmark::State& state) {
  Kokkos::View<double *> a("a", 0), b("b", 0);
  for (auto _ : state) {
    Kokkos::deep_copy(b, a);
  }
}
// Register the function as a benchmark
BENCHMARK(BM_deepcopy)->UseRealTime();

KTS_BENCHMARK_MAIN();