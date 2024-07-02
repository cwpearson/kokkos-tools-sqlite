#pragma once

#include <iostream>
#include <filesystem>

#include <benchmark/benchmark.h>

#define KTS_BENCHMARK_MAIN() \
int main(int argc, char** argv) {\
    setenv("KTS_SQLITE_PATH", "kts_perf_test.sqlite", false);\
    const char *rawPath = std::getenv("KTS_SQLITE_PATH");\
    if (rawPath) {\
        std::cerr << __FILE__ << ":" << __LINE__ << " remove " << rawPath << " before test...\n";\
        std::filesystem::remove(std::filesystem::path(rawPath));\
    }\
    Kokkos::initialize();\
    ::benchmark::Initialize(&argc, argv);\
    ::benchmark::RunSpecifiedBenchmarks();\
    ::benchmark::Shutdown();\
    Kokkos::finalize();\
    if (rawPath) {\
        std::cerr << __FILE__ << ":" << __LINE__ << " remove " << rawPath << " after test...\n";\
        std::filesystem::remove(std::filesystem::path(rawPath));\
    }\
    return 0;\
}
