#include <Kokkos_Core.hpp>

#include "perf_main.hpp"

static void BM_alloc(benchmark::State &state) {
  Kokkos::DefaultExecutionSpace space;
  for (auto _ : state) {
    Kokkos::View<char *> v(
        Kokkos::view_alloc(space, Kokkos::WithoutInitializing, "v"), 1);
  }
}
// Register the function as a benchmark
BENCHMARK(BM_alloc)->UseRealTime();

KTS_BENCHMARK_MAIN();