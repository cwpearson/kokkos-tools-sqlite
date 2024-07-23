#include <Kokkos_Core.hpp>

#include "perf_main.hpp"

static void BM_alloc(benchmark::State &state) {
  Kokkos::DefaultExecutionSpace space;
  for (auto _ : state) {
    Kokkos::View<char *> v(
        Kokkos::view_alloc(space, Kokkos::WithoutInitializing, "v"),
        state.range(0));
  }
}
// Register the function as a benchmark
BENCHMARK(BM_alloc)
    ->UseRealTime()
    ->Arg(8)
    ->Arg(64)
    ->Arg(512)
    ->Arg(4 << 10)
    ->Arg(8 << 10);

KTS_BENCHMARK_MAIN();