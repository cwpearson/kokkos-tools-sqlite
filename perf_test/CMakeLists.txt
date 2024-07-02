include(FetchContent)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
  benchmark
  URL https://github.com/google/benchmark/archive/refs/tags/v1.8.3.zip
)
FetchContent_Declare(Kokkos
  GIT_REPOSITORY    git@github.com:kokkos/kokkos.git
  GIT_TAG           master
)
FetchContent_MakeAvailable(benchmark Kokkos)

function (kts_add_bench tgt)
  add_executable(${tgt} ${ARGN})
  target_link_libraries(${tgt} Kokkos::kokkos benchmark::benchmark)
  add_test(NAME ${tgt} COMMAND ${tgt})
  set_property(TEST ${tgt} PROPERTY ENVIRONMENT "KOKKOS_TOOLS_LIBS=${CMAKE_BINARY_DIR}/libkts.so")
endfunction()

kts_add_bench(perf_spans perf_spans.cpp)
