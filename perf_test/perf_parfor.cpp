#include <Kokkos_Core.hpp>

#include "perf_main.hpp"


static void BM_parfor(benchmark::State& state) {
  for (auto _ : state) {
    Kokkos::parallel_for(0, KOKKOS_LAMBDA(int){});
  }
}
// Register the function as a benchmark
BENCHMARK(BM_parfor)->UseRealTime();

KTS_BENCHMARK_MAIN();